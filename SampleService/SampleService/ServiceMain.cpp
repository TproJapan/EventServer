///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�v���O�����̃T���v��
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <Windows.h>
#include "CLog.h"
#include "../src/TcpServer.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
const char* LOGFILE_NAME = "C:\\tmp\\SampleService.log";
LPWSTR SERVICE_NAME = (LPWSTR)L"SampleService";

///////////////////////////////////////////////////////////////////////////////
// �O���ϐ�
///////////////////////////////////////////////////////////////////////////////
CLog logObj;
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

///////////////////////////////////////////////////////////////////////////////
// ���C������
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	bool bRet;
	
	// ���O�t�@�C�����쐬
	bRet = logObj.open(LOGFILE_NAME);
	logObj.log("main started");

	// �T�[�r�X���C���֐���SCM�ɓo�^����
	bRet = registServiceMain();
	if (!bRet) {
		logObj.log("registServiceMain error.");
		return -1;
	}

	logObj.log("main ended");
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

	logObj.log("%s started.", __FUNCTION__);

	// SCM�ɓo�^����
	BOOL success = StartServiceCtrlDispatcher(serviceTable);
	if (!success)
	{
		logObj.log("StartServiceCtrlDispatcher error. (%d)", GetLastError());
		return false;
	}

	logObj.log("%s ended.", __FUNCTION__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SCM�ɃT�[�r�X��main�֐���o�^����
///////////////////////////////////////////////////////////////////////////////
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	logObj.log("%s started.", __FUNCTION__);

	// �����ɓo�^�֐����Ăяo��
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
													(LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		logObj.log("RegisterServiceCtrlHandler error.", GetLastError());
		terminate(GetLastError());
		return;
	}

	logObj.log("RegisterServiceCtrlHandler normal ended.");

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		logObj.log("SendStatusToSCM error.");
		terminate(GetLastError());
		return;
	}

	// �I���C�x���g���쐬����
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);
	if (!terminateEvent)
	{
		logObj.log("CreateEvent error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}
	logObj.log("CreateEvent normal ended");

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		logObj.log("SendStatusToSCM(2) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �J�n�p�����[�^�̗L�����`�F�b�N����
	/*
	if (argc == 2)
	{
		int temp = atoi(argv[1]);
		if (temp < 1000)
		{
			beepDelay = 2000;
		}
		else
		{
			beepDelay = temp;
		}
	}*/

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		logObj.log("SendStatusToSCM(3) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �T�[�r�X���̂��J�n����
	success = InitService();
	if (!success)
	{
		logObj.log("InitService error");
		terminate(GetLastError());
		return;
	}

	// ���̎��_�ŃT�[�r�X�͎��s��ԂɂȂ��Ă���B
	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		logObj.log("SendStatusToSCM(4) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �I���V�O�i����ҋ@���A��������o������I������B
	logObj.log("Before WaitForSingleObject");
	WaitForSingleObject(terminateEvent, INFINITE);
	logObj.log("After WaitForSingleObject");

	terminate(0);

	logObj.log("%s ended.", __FUNCTION__);
	return;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain�֐��ŃG���[�����������ꍇ�̏����Ƃ��āA
// �N���[���A�b�v���s���A�T�[�r�X���J�n���Ȃ��������Ƃ�
// SCM�ɒʒm����B
///////////////////////////////////////////////////////////////////////////////
VOID terminate(DWORD error)
{
	logObj.log("%s started.", __FUNCTION__);

	// terminateEvent�n���h�����쐬����Ă���ꍇ�͕���
	if (terminateEvent) {
		logObj.log("terminateEvent���N���[�Y.");
		CloseHandle(terminateEvent);
	}

	// �T�[�r�X����~�������Ƃ�ʒm���邽�߂�SCM�Ƀ��b�Z�[�W�𑗐M����
	if (serviceStatusHandle)
	{
		logObj.log("SCM�ɃT�[�r�X�̒�~��ʒm.");
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// �X���b�h���J�n����Ă���ꍇ�́A������I������
	if (threadHandle)
	{
		logObj.log("threadHandle���N���[�Y.");
		CloseHandle(threadHandle);
	}

	// serviceStatusHandle�͕���K�v���Ȃ��B

	logObj.log("%s ended.", __FUNCTION__);
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

	logObj.log("%s started.", __FUNCTION__);

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
		logObj.log("SetServiceStatus error. %d", GetLastError());
		StopService();
	}

	logObj.log("%s ended.", __FUNCTION__);
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

	logObj.log("%s started.", __FUNCTION__);

	switch (controlCode)
	{
		// �J�n����SeviceMain�֐����Ăяo�����̂�
		// START(�J�n)�I�v�V�����͂Ȃ�

		// �T�[�r�X���~����
	case SERVICE_CONTROL_STOP:
		logObj.log("SERVICE_CONTROL_STOP");
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
		logObj.log("SERVICE_CONTROL_PAUSE");
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
		logObj.log("SERVICE_CONTROL_CONTINUE");
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
		logObj.log("SERVICE_CONTROL_INTERROGATE");
		break;

		// �V���b�g�_�E�����ɂ͉������Ȃ��B�����ŃN���[���A�b�v��
		// �s�����Ƃ��ł��邪�A���ɂ��΂₭�s��Ȃ���΂Ȃ�Ȃ��B
	case SERVICE_CONTROL_SHUTDOWN:
		// �V���b�g�_�E�����ɂ͉������Ȃ��B
		logObj.log("SERVICE_CONTROL_SHUTDOWN");
		return;

	default:
		logObj.log("default");
		break;
	}

	success = SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);

	logObj.log("%s ended.", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�����������邽�߂ɂ��̃X���b�h���J�n����B
///////////////////////////////////////////////////////////////////////////////
BOOL InitService()
{
	DWORD id;

	logObj.log("%s started.", __FUNCTION__);

	// �T�[�r�X�̃X���b�h���J�n����
	threadHandle = CreateThread(0, 0,
								(LPTHREAD_START_ROUTINE)ServiceThread,
								0, 0, &id);
	if (threadHandle == 0)
	{
		logObj.log("CreateThread error. %d", GetLastError());
		return FALSE;
	}


	logObj.log("CreateThread normal ended", 0);
	runningService = TRUE;

	logObj.log("%s ended.", __FUNCTION__);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain�֐������������邱�Ƃɂ���āA�T�[�r�X���~����B
///////////////////////////////////////////////////////////////////////////////
VOID StopService()
{
	logObj.log("%s started.", __FUNCTION__);

	runningService = FALSE;

	//Tcp�T�[�o��~�֐��Ăяo��
	StopTcpServer();
	logObj.log("Tcp�T�[�o��~���J�n���܂���");

	// ServiceMain�֐��������Ԃ����Ƃ��ł���悤�ɂ��邽�߂�
	// ServiceMain�֐�����������̂�h�~���Ă���C�x���g���Z�b�g����B
	SetEvent(terminateEvent);

	logObj.log("%s ended.", __FUNCTION__);
	return;
}

///////////////////////////////////////////////////////////////////////////////
// �ꎞ��~���ꂽ�T�[�r�X���ĊJ����
///////////////////////////////////////////////////////////////////////////////
VOID ResumeService()
{
	logObj.log("%s started.", __FUNCTION__);
	pauseServise = FALSE;
	ResumeThread(threadHandle);
	logObj.log("%s ended.", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X���ꎞ��~����
///////////////////////////////////////////////////////////////////////////////
VOID PauseService()
{
	logObj.log("%s started.", __FUNCTION__);
	pauseServise = TRUE;
	SuspendThread(threadHandle);
	logObj.log("%s ended.", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�{���̏���
///////////////////////////////////////////////////////////////////////////////
DWORD ServiceThread(LPDWORD param)
{
	logObj.log("%s started.", __FUNCTION__);

//	while (1)
//	{
//		logObj.log("�T�[�r�X�A�T�[�r�X!!!");
//		Sleep(2000);
//	}
	
	int result = Tcpserver();
	if(result != 0)
	{
		logObj.log("%s ended with error %d!", "Tcpserver()", result);
	}
	
	logObj.log("%s ended.", __FUNCTION__);
	return 0;
}
