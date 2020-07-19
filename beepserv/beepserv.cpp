//#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>

using namespace std;
// �O���[�o���ϐ�

// �T�[�r�X�̖��O
//char* SERVICE_NAME = (char*)"BeepService";
char* SERVICE_NAME = (char*)"beep_srv";
//#define SERVICE_NAME L"BeepService"

// ServiceMain����������̂�h�~���邽�߂Ɏg�p����C�x���g�n���h��
// Handle�Ƃ͍\���̂ւ̃|�C���^�B�����o�ϐ��ɒl���������݂ɍs���l�ƁA�C�x���g����������B
// Handle�ϐ��̈�̂ǂ����ɃC�x���g�ԍ����Z�b�g����̈悪����B����ɂ���ăC�x���g�Ƃ̌��ѕt�����ł���
// �C�x���g��WaitForSingleEvent�Ƃ�WaitForMultiEvent�ő҂B
// �C�x���g���������� or �n���h�����N���[�Y������Wait��������
// �C�x���g�̓v���O������́u���l�v�ň�����
// �n���h���̓C�x���g���N���Ă���s���鏈�����e�B�֐���
HANDLE terminateEvent = NULL;


// ��ԏ���SCM�Ƃ̊ԂŒʒm���������߂Ɏg�p����n���h��
// RegisterServiceCtrlHandler�֐��ɂ���č쐬�����
SERVICE_STATUS_HANDLE serviceStatusHandle;

// �r�[�v����炷�~���b�P�ʂ̊Ԋu
int beepDelay = 2000;

// �T�[�r�X�̌��݂̏�Ԃ��i�[����t���O
BOOL pauseServise = FALSE;
BOOL runningService = FALSE;

// ���ۂ̏������s�����߂̃X���b�h�n���h��
// CreateThread�֐��̖߂�l��Handle�^�̃X���b�h�n���h��
// Unix���Ɩ߂�l�̓X���b�hid
HANDLE threadHandle = 0;

void log(const char* message, DWORD err)
{
	FILE* fp = fopen("c:\\tmp\\service_log.txt", "a");
	fprintf(fp, "%s. err=%ld\n", message, err);
	fclose(fp);
}

void ErrorHandler(char* s, DWORD err)
{
	cout << s << endl;
	cout << "�G���[�ԍ�: " << err << endl;
	ExitProcess(err);

}
//���̕���LPTHREAD_START_ROUTINE�^�̊֐��|�C���^��v������Ă���̂Ŏ��삷��
//LPTHREAD_START_ROUTINE�^�Ƃ����͈̂�����LPDWORD�^�A�߂�l��DWORD�ł���K�v������
DWORD ServiceThread(LPDWORD param)
{
	while (1)
	{
		log("ServiceThread", 0);
		Beep(200, 200);
		Sleep(beepDelay);
	}

	return 0;
}

SOCKET fd;

DWORD waitReceive(LPDWORD param)
{
	char    buf[1024];
	int recvSize;
	int nRet;
	HANDLE  hEvent = WSACreateEvent();
	WSANETWORKEVENTS    events;

	WSAEventSelect(fd, hEvent, FD_READ | FD_CLOSE);

	while (1)
	{
		//�C�x���g�҂�
		nRet = WSAWaitForMultipleEvents(1, &hEvent, FALSE, WSA_INFINITE, FALSE);

		if (nRet == WSA_WAIT_FAILED)
		{
			log("!!!!!!!!!!!!!!!!!!!!",0);
			break;
		}

		//�C�x���g�̒���
		if (WSAEnumNetworkEvents(fd, hEvent, &events) == SOCKET_ERROR)
		{
			log("???",0);
		}
		else
		{
			//CLOSE
			if (events.lNetworkEvents & FD_CLOSE)
			{
				log("close", 0);
				closesocket(fd);
				break;
			}
			//READ
			if (events.lNetworkEvents & FD_READ)
			{
				recvSize = recv(fd, buf, 1024, 0);
				printf("Received %d bytes\n", recvSize);
			}
		}
	}

	WSACloseEvent(hEvent);//�C�x���g�N���[�Y

	return 0;
}

// �T�[�r�X�����������邽�߂ɂ��̃X���b�h���J�n����B
BOOL InitService()
{
	struct sockaddr_in	addr;
	int	addrlen;
	//HANDLE	hThread;
	DWORD	threadId;
	WORD	wVersionRequested = MAKEWORD(2, 2);
	WSADATA	wsaData;

	WSAStartup(wVersionRequested, &wsaData);	//Winsock������

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		log("socket", GetLastError());
		return -1;
	}

	//�|�[�g�̐ݒ�
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(20319);

	addrlen = sizeof(addr);
	//BIND
	if (bind(fd, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
	{
		log("bind", GetLastError());
		return -1;
	}

	//LISTEN
	if (listen(fd, 0) == SOCKET_ERROR)
	{
		log("listen", GetLastError());
		return -1;
	}

	log("Listening", 0);

	//ACCEPT
	if ((fd = accept(fd, (struct sockaddr*)&addr, &addrlen)) == INVALID_SOCKET)
	{
		log("accept", GetLastError());
		return -1;
	}

	log("Accepted", 0);

	//DWORD id;

	// �T�[�r�X�̃X���b�h���J�n����
	threadHandle = CreateThread(0, 0,
		(LPTHREAD_START_ROUTINE)waitReceive,
		0, 0, &threadId);
	if (threadHandle == 0)
	{
		return FALSE;
	}
	else
	{
		runningService = TRUE;
		return TRUE;
	}
}

// �ꎞ��~���ꂽ�T�[�r�X���ĊJ����
VOID ResumeService()
{
	pauseServise = FALSE;
	ResumeThread(threadHandle);
}

// �T�[�r�X���ꎞ��~����
VOID PauseService()
{
	pauseServise = TRUE;
	SuspendThread(threadHandle);
}

// ServiceMain�֐������������邱�Ƃɂ���āA�T�[�r�X��
// ��~����B
VOID StopService()
{
	runningService = FALSE;

	// ServiceMain�֐��������Ԃ����Ƃ��ł���悤�ɂ��邽�߂�
	// ServiceMain�֐�����������̂�h�~���Ă���C�x���g���Z�b�g����B
	SetEvent(terminateEvent);
}

// ���̊֐��́ASetServiceStatus�֐��ɂ����
// �T�[�r�X�̏�Ԃ��X�V���鏈�����܂Ƃ߂Ď��s����B
BOOL SendStatusToSCM(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint)
{
	BOOL success;
	SERVICE_STATUS serviceStatus;

	// SERVICE_STATUS�\���̂̂��ׂẴ����o�ɒl��ݒ肷��B
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = dwCurrentState;

	// ���炩�̏������s���Ă���ꍇ�́A�R���g���[���C�x���g���󂯎��Ȃ��B
	// ����ȊO�̏ꍇ�́A�R���g���[���C�x���g���󂯎��B
	if (dwCurrentState == SERVICE_START_PENDING)
	{
		serviceStatus.dwControlsAccepted = 0;
	}
	else
	{
		serviceStatus.dwControlsAccepted =
			SERVICE_ACCEPT_STOP |
			SERVICE_ACCEPT_PAUSE_CONTINUE |
			SERVICE_ACCEPT_SHUTDOWN;
	}

	// ����̏I���R�[�h����`����Ă���ꍇ��
	// win32�̏I���R�[�h�𐳂����ݒ肷��B
	if (dwServiceSpecificExitCode == 0)
	{
		serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
	}
	else
	{
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	}
	serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

	serviceStatus.dwCheckPoint = dwCheckPoint;
	serviceStatus.dwWaitHint = dwWaitHint;

	// ��ԃ��R�[�h��SCM�ɓn���B
	success = SetServiceStatus(serviceStatusHandle, &serviceStatus);

	if (!success)
	{
		StopService();
	}

	return success;
}

// �T�[�r�X�R���g���[���}�l�[�W������󂯎�����C�x���g��
// �f�B�X�p�b�`����B
VOID ServiceCtrlHandler(DWORD controlCode)
{
	DWORD currentState = 0;
	BOOL success;

	switch (controlCode)
	{
		// �J�n����SeviceMain�֐����Ăяo�����̂�
		// START(�J�n)�I�v�V�����͂Ȃ�

		// �T�[�r�X���~����
	case SERVICE_CONTROL_STOP:
		log("SERVICE_CONTROL_STOP", 0);
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
		log("SERVICE_CONTROL_PAUSE", 0);
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
		log("SERVICE_CONTROL_CONTINUE", 0);
		if (runningService && !pauseServise)
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
		break;

		// �V���b�g�_�E�����ɂ͉������Ȃ��B�����ŃN���[���A�b�v��
		// �s�����Ƃ��ł��邪�A���ɂ��΂₭�s��Ȃ���΂Ȃ�Ȃ��B
		// ����ȏ����̓V���b�g�_�E���܂ł̗P�\�ɊԂɍ��킸OS�ɎE�����
	case SERVICE_CONTROL_SHUTDOWN:
		// �V���b�g�_�E�����ɂ͉������Ȃ��B
		return;

	default:
		break;
	}

	SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
}


// ServiceMain�֐��ŃG���[�����������ꍇ�̏����Ƃ��āA
// �N���[���A�b�v���s���A�T�[�r�X���J�n���Ȃ��������Ƃ�
// SCM�ɒʒm����B�N���[���A�b�v�Ƃ͓��I�Ɏ擾�������\�[�X���J��������ڑ���؂����肷�鎖�E
//����̏ꍇ��terminate�n���h�������beep�̃��[�J�[�X���b�h����鎖�B
VOID terminate(DWORD error)
{
	// terminateEvent�n���h�����쐬����Ă���ꍇ�́A��������
	if (terminateEvent) {
		CloseHandle(terminateEvent);
	}

	// �T�[�r�X����~�������Ƃ�ʒm���邽�߂�
	// SCM�Ƀ��b�Z�[�W�𑗐M����
	if (serviceStatusHandle)
	{
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// �X���b�h���J�n����Ă���ꍇ�́A������I������
	if (threadHandle)
	{
		CloseHandle(threadHandle);

	}
	// serviceStatusHandle�͕���K�v���Ȃ��B
}

// ServiceMain�֐��́ASCM���T�[�r�X���J�n���鎞�ɌĂяo�����B
// ServiceMain�֐���return(�����Ԃ�)�ƁA�T�[�r�X�͒�~����B
// ���������Ċ֐��̎��s���I�����钼�O�܂ŁAServiceMain�֐���
// �C�x���g��ҋ@���Ă����Ԃɂ��Ă����A��~���鎞�ɂȂ�����
// ���̃C�x���g���V�O�i����ԂɃZ�b�g����B(�V�O�i�����:�C�x���g�����������u�Ԃ̎�)
// �܂��G���[�����������ꍇ�ɂ̓T�[�r�X���J�n�ł��Ȃ��̂ŁA
// �G���[�����������ꍇ�ɂ�ServiceMain�֐���return(�����Ԃ�)�B
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;

	// �����ɓo�^�֐����Ăяo��
	log("Before RegisterServiceCtrlHandler", 0);

	serviceStatusHandle = RegisterServiceCtrlHandler(
		SERVICE_NAME,
		(LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		log("RegisterServiceCtrlHandler error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		log("SendStatusToSCM(1) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �I���C�x���g���쐬����
	log("Before CreateEvent", 0);
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);//���:�Z�L�����e�B���:�蓮���Z�b�g��O:������ԑ�l�C�x���g�̖��O(������ւ̃|�C���^)
	//�{���̎g������SetEvent(�C�x���g��)�ő�O������True�ɂȂ��ăC�x���g��emit����
	if (!terminateEvent)
	{
		log("CreateEvent error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		log("SendStatusToSCM(2) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �J�n�p�����[�^�̗L�����`�F�b�N����
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
	}

	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		log("SendStatusToSCM(3) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �T�[�r�X���̂��J�n����
	success = InitService();
	if (!success)
	{
		log("InitService error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// ���̎��_�ŃT�[�r�X�͎��s��ԂɂȂ��Ă���B
	// �i�s�󋵂�SCM�ɒʒm����
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		log("SendStatusToSCM(4) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// �I���V�O�i����ҋ@���A��������o������I������B
	log("Before WaitForSingleObject", 0);
	WaitForSingleObject(terminateEvent, INFINITE);

	terminate(0);
}

int main(VOID)
{
	SERVICE_TABLE_ENTRY	serviceTable[] =
	{
		{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL,NULL}
	};
	BOOL success;

	// SCM�ɓo�^����
	log("check-01", 0);
	success = StartServiceCtrlDispatcher(serviceTable);
	if (!success)
	{
		log("StartServiceCtrlDispatcher�ŃG���[������", GetLastError());
		ErrorHandler((char*)"StartServiceCtrlDispatcher�ŃG���[������", GetLastError());
	}

	return 0;
}