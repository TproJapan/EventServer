#ifdef _WIN64
///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�v���O�����̃T���v��
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "../src/BoostLog.h"
#include "../src/TcpServer.h"
#include <Windows.h>
#include <stdio.h>
#include <WinSock2.h>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
const char* LOGFILE_NAME = "C:\\tmp\\SampleService.log";
LPWSTR SERVICE_NAME = (LPWSTR)L"SampleService";

///////////////////////////////////////////////////////////////////////////////
// �O���ϐ�
///////////////////////////////////////////////////////////////////////////////
//CLog logObj;
SERVICE_STATUS_HANDLE serviceStatusHandle;
// ��ԏ���SCM�Ƃ̊ԂŒʒm���������߂Ɏg�p����n���h��
// RegisterServiceCtrlHandler�֐��ɂ���č쐬�����

// ServiceMain����������̂�h�~���邽�߂Ɏg�p����C�x���g
HANDLE terminateEvent = NULL;

// ���ۂ̏������s�����߂̃X���b�h
HANDLE threadHandle = 0;

// �T�[�r�X�̌��݂̏�Ԃ��i�[����t���O
BOOL pauseServise = FALSE;
BOOL runningService = FALSE;

///////////////////////////////////////////////////////////////////////////////
// �v���g�^�C�v�錾
///////////////////////////////////////////////////////////////////////////////
bool registServiceMain();
VOID ServiceMain(DWORD argc, LPSTR* argv);
VOID ServiceCtrlHandler(DWORD controlCode);
VOID terminate(DWORD error);
BOOL SendStatusToSCM(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint);
BOOL InitService();
VOID StopService();
VOID ResumeService();
VOID PauseService();
DWORD ServiceThread(LPDWORD param);
TcpServer* tcpServer;
///////////////////////////////////////////////////////////////////////////////
// ���C������
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
	logging::add_common_attributes();

	bool bRet;

	writeLog(2, "main started, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	// �T�[�r�X���C���֐���SCM�ɓo�^����
	bRet = registServiceMain();
	if (!bRet) {
		writeLog(4, "registServiceMain error. (%ld),%s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		return -1;
	}

	writeLog(2, "main ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// SCM�ɃT�[�r�X��main�֐���o�^����
///////////////////////////////////////////////////////////////////////////////
bool registServiceMain()
{
	SERVICE_TABLE_ENTRY	serviceTable[] =
	{
		{SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL,NULL}
	};

	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// SCM�ɓo�^����
	BOOL success = StartServiceCtrlDispatcher(serviceTable);
	if (!success)
	{
		writeLog(4, "StartServiceCtrlDispatcher error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return false;
	}

	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SCM�ɃT�[�r�X��main�֐���o�^����
///////////////////////////////////////////////////////////////////////////////
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// �����ɓo�^�֐����Ăяo��
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
		(LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		writeLog(4, "RegisterServiceCtrlHandler error. %s %d %s\n", __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	writeLog(2, "RegisterServiceCtrlHandler normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		writeLog(2, "SendStatusToSCM error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		terminate(GetLastError());
		return;
	}

	// �I���C�x���g���쐬����
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);
	if (!terminateEvent)
	{
		writeLog(4, "CreateEvent error. %d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	writeLog(2, "CreateEvent normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		writeLog(4, "SendStatusToSCM(2) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		writeLog(4, "SendStatusToSCM(3) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	// �T�[�r�X���̂��J�n����
	success = InitService();
	if (!success)
	{
		writeLog(4, "InitService error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		terminate(GetLastError());
		return;
	}

	// ���̎��_�ŃT�[�r�X�͎��s��ԂɂȂ��Ă���B
	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		writeLog(4, "SendStatusToSCM(4) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	WaitForSingleObject(terminateEvent, INFINITE);
	writeLog(2, "After WaitForSingleObject. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	terminate(0);

	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain�֐��ŃG���[�����������ꍇ�̏����Ƃ��āA
// �N���[���A�b�v���s���A�T�[�r�X���J�n���Ȃ��������Ƃ�
// SCM�ɒʒm����B
///////////////////////////////////////////////////////////////////////////////
VOID terminate(DWORD error)
{
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// terminateEvent�n���h�����쐬����Ă���ꍇ�͕���
	if (terminateEvent) {
		writeLog(2, "terminateEvent���N���[�Y. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		CloseHandle(terminateEvent);
	}

	// �T�[�r�X����~�������Ƃ�ʒm���邽�߂�SCM�Ƀ��b�Z�[�W�𑗐M����
	if (serviceStatusHandle)
	{
		writeLog(2, "SCM�ɃT�[�r�X�̒�~��ʒm. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// �X���b�h���J�n����Ă���ꍇ�́A������I������
	if (threadHandle)
	{
		writeLog(2, "threadHandle���N���[�Y. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		CloseHandle(threadHandle);
	}

	// serviceStatusHandle�͕���K�v���Ȃ��B

	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return;
}

///////////////////////////////////////////////////////////////////////////////
// ���̊֐��́ASetServiceStatus�֐��ɂ����
// �T�[�r�X�̏�Ԃ��X�V���鏈�����܂Ƃ߂Ď��s����B
///////////////////////////////////////////////////////////////////////////////
BOOL SendStatusToSCM(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint)
{
	BOOL success;
	SERVICE_STATUS status;

	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// SERVICE_STATUS�\���̂̂��ׂẴ����o�ɒl��ݒ肷��B
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = dwCurrentState;

	// ���炩�̏������s���Ă���ꍇ�́A�R���g���[���C�x���g���󂯎��Ȃ��B
	// ����ȊO�̏ꍇ�́A�R���g���[���C�x���g���󂯎��B
	if (dwCurrentState == SERVICE_START_PENDING)
	{
		status.dwControlsAccepted = 0;
	}
	else
	{
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
			SERVICE_ACCEPT_PAUSE_CONTINUE |
			SERVICE_ACCEPT_SHUTDOWN;
	}

	// ����̏I���R�[�h����`����Ă���ꍇ��
	// win32�̏I���R�[�h�𐳂����ݒ肷��B
	if (dwServiceSpecificExitCode == 0)
	{
		status.dwWin32ExitCode = dwWin32ExitCode;
	}
	else
	{
		status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	}

	status.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	status.dwCheckPoint = dwCheckPoint;
	status.dwWaitHint = dwWaitHint;

	// ��ԃ��R�[�h��SCM�ɓn���B
	success = SetServiceStatus(serviceStatusHandle, &status);
	if (!success)
	{
		writeLog(4, "SetServiceStatus error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		StopService();
	}

	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�R���g���[���}�l�[�W������󂯎�����C�x���g��
// �f�B�X�p�b�`����B
///////////////////////////////////////////////////////////////////////////////
VOID ServiceCtrlHandler(DWORD controlCode)
{
	DWORD currentState = 0;
	BOOL success;

	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	switch (controlCode)
	{
		// �J�n����SeviceMain�֐����Ăяo�����̂�
		// START(�J�n)�I�v�V�����͂Ȃ�

		// �T�[�r�X���~����
	case SERVICE_CONTROL_STOP:
		writeLog(2, "SERVICE_CONTROL_STOP. %s %d %s\n", __FILENAME__, __LINE__, __func__);

		currentState = SERVICE_STOP_PENDING;
		// ���݂̏�Ԃ�SCM�ɒʒm����
		success = SendStatusToSCM(SERVICE_STOP_PENDING,
			NO_ERROR, 0, 1, 5000);
		// �������Ȃ������ꍇ�A���ɉ������Ȃ�

		// �T�[�r�X���~����
		StopService();
		return;

		// �T�[�r�X���ꎞ��~����
	case SERVICE_CONTROL_PAUSE:
		writeLog(2, "SERVICE_CONTROL_PAUSE. %s %d %s\n", __FILENAME__, __LINE__, __func__);

		if (runningService && !pauseServise)
		{
			// ���݂̏�Ԃ�SCM�ɒʒm����
			success = SendStatusToSCM(SERVICE_PAUSE_PENDING,
				NO_ERROR, 0, 1, 1000);
			PauseService();
			currentState = SERVICE_PAUSED;
		}
		break;

		// �ꎞ��~����ĊJ����
	case SERVICE_CONTROL_CONTINUE:
		writeLog(2, "SERVICE_CONTROL_CONTINUE. %s %d %s\n", __FILENAME__, __LINE__, __func__);

		if (runningService && pauseServise)
		{
			// ���݂̏�Ԃ�SCM�ɒʒm����
			success = SendStatusToSCM(SERVICE_CONTINUE_PENDING,
				NO_ERROR, 0, 1, 1000);
			ResumeService();
			currentState = SERVICE_RUNNING;
		}
		break;

		// ���݂̏�Ԃ��X�V����
	case SERVICE_CONTROL_INTERROGATE:
		// ����switch���̌�ɍs�ɐi�݁A��Ԃ𑗐M����
		writeLog(2, "SERVICE_CONTROL_INTERROGATE. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		break;

		// �V���b�g�_�E�����ɂ͉������Ȃ��B�����ŃN���[���A�b�v��
		// �s�����Ƃ��ł��邪�A���ɂ��΂₭�s��Ȃ���΂Ȃ�Ȃ��B
	case SERVICE_CONTROL_SHUTDOWN:
		// �V���b�g�_�E�����ɂ͉������Ȃ��B
		writeLog(2, "SERVICE_CONTROL_SHUTDOWN. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return;

	default:
		writeLog(2, "default. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		break;
	}

	success = SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�����������邽�߂ɂ��̃X���b�h���J�n����B
///////////////////////////////////////////////////////////////////////////////
BOOL InitService()
{
	DWORD id;
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// �T�[�r�X�̃X���b�h���J�n����
	threadHandle = CreateThread(0, 0,
		(LPTHREAD_START_ROUTINE)ServiceThread,
		0, 0, &id);
	if (threadHandle == 0)
	{
		writeLog(4, "CreateThread error. %d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		return FALSE;
	}

	writeLog(2, "CreateThread normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
	runningService = TRUE;
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain�֐������������邱�Ƃɂ���āA�T�[�r�X���~����B
///////////////////////////////////////////////////////////////////////////////
VOID StopService()
{
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	runningService = FALSE;
	writeLog(2, "TcpServerMainEnd�̑ҋ@���J�n���܂���. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// ���݂̏�Ԃ�SCM�ɒʒm����
	BOOL success = SendStatusToSCM(SERVICE_STOP_PENDING,
		NO_ERROR, 0, 1, 30000);

	//WaitForSingleObject(TcpServerMainEnd,INFINITE);
#if 1
	TcpServerMainEnd = CreateEvent(0, TRUE, FALSE, 0);
	if (!TcpServerMainEnd)
	{
		writeLog(4, "CreateEvent error.  %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	writeLog(2, "CreateEvent normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	//Tcp�T�[�o��~�֐��Ăяo��
	writeLog(2, "Tcp�T�[�o��~���J�n���܂���. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	if (tcpServer != NULL) tcpServer->StopTcpServer();

	DWORD dwRet = WaitForSingleObject(TcpServerMainEnd, 20000);
	string strRet = "";
	switch (dwRet) {
	case WAIT_ABANDONED:
		strRet = "WAIT_ABANDONED";
		break;
	case WAIT_OBJECT_0:
		strRet = "WAIT_OBJECT_0";
		break;
	case WAIT_TIMEOUT:
		strRet = "WAIT_TIMEOUT";
		break;
	case WAIT_FAILED:
		strRet = "WAIT_FAILED";
		break;
	default:
		strRet = "unknown";
		break;
	}

	writeLog(2, "strRet = %s. %s %d %s\n", strRet.c_str(), __FILENAME__, __LINE__, __func__);

	if (dwRet == WAIT_FAILED) {
		writeLog(4, "WaitForSingleObject error.code=%d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

	}
#endif

	writeLog(2, "TcpServerMainEnd�̑ҋ@���I�����܂���. %s %d %s\n", __FILENAME__, __LINE__, __func__);

#if 1
	if (TcpServerMainEnd)
	{
		CloseHandle(TcpServerMainEnd);
		writeLog(2, "CloseHandle ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
	}
#endif

	// ServiceMain�֐�����������C�x���g���Z�b�g����B
	SetEvent(terminateEvent);
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return;
}

///////////////////////////////////////////////////////////////////////////////
// �ꎞ��~���ꂽ�T�[�r�X���ĊJ����
///////////////////////////////////////////////////////////////////////////////
VOID ResumeService()
{
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	pauseServise = FALSE;
	ResumeThread(threadHandle);
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X���ꎞ��~����
///////////////////////////////////////////////////////////////////////////////
VOID PauseService()
{
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	pauseServise = TRUE;
	SuspendThread(threadHandle);
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�{���̏���
///////////////////////////////////////////////////////////////////////////////
DWORD ServiceThread(LPDWORD param)
{
	writeLog(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	const char* name = "eventserver";
	const char* protocol = "tcp";

	WSADATA	WsaData;
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup(wVersionRequested, &WsaData) != 0) {
		writeLog(4, "WSAStartup() error. code=%d %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		return -1;
	}

	struct servent* pServent = NULL;
	pServent = getservbyname(name, protocol);
	int port = 5000;//Default value
	if (pServent == NULL) 
	{
		writeLog(4, "getservbyname error, uses 5000 for Port %s %d %s\n", __FILENAME__, __LINE__, __func__);
	}else {
		port = ntohs(pServent->s_port);
		writeLog(2, "Uses %d for Port. %s %d %s\n", port, __FILENAME__, __LINE__, __func__);
	}

	try {
		tcpServer = new TcpServer(port);
	}
	catch (int e) 
	{
		writeLog(4, "tcpServer create error. code=%d %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		return -1;
	}

	int result = tcpServer->Func();

	if (result != 0)
	{
		writeLog(4, "Tcpserver() ended with error %d! %s %d %s\n", result, __FILENAME__, __LINE__, __func__);
	}

	delete tcpServer;
	
	writeLog(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return 0;
}
#endif