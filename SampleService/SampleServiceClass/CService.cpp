#include "CService.h"
#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
//const char* LOGFILE_NAME = "C:\\tmp\\SampleService.log";
LPWSTR SERVICE_NAME = (LPWSTR)L"SampleService";


///////////////////////////////////////////////////////////////////////////////
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////
//CService::CService(CLog& log) :logObj(log)
CService::CService()
{

}

///////////////////////////////////////////////////////////////////////////////
// �f�X�g���N�^
///////////////////////////////////////////////////////////////////////////////
CService::~CService()
{

}


///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�̊J�n
///////////////////////////////////////////////////////////////////////////////
bool CService::run()
{
	bool bRet;

	// �T�[�r�X���C���֐���SCM�ɓo�^����
	bRet = registServiceMain();
	if (!bRet) {
		logObj.log("registServiceMain error.");
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// SCM�ɃT�[�r�X��main�֐���o�^����
///////////////////////////////////////////////////////////////////////////////
bool CService::registServiceMain()
{
	SERVICE_TABLE_ENTRY	serviceTable[] =
	{
		{SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)(ServiceMain)},
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
VOID CService::ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	//logObj.log("%s started.", __FUNCTION__);

#if 0
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
#endif
	return;
}
