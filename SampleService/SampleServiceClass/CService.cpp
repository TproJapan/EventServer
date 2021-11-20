#include "CService.h"
#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
//const char* LOGFILE_NAME = "C:\\tmp\\SampleService.log";
LPWSTR SERVICE_NAME = (LPWSTR)L"SampleService";


///////////////////////////////////////////////////////////////////////////////
// コンストラクタ
///////////////////////////////////////////////////////////////////////////////
//CService::CService(CLog& log) :logObj(log)
CService::CService()
{

}

///////////////////////////////////////////////////////////////////////////////
// デストラクタ
///////////////////////////////////////////////////////////////////////////////
CService::~CService()
{

}


///////////////////////////////////////////////////////////////////////////////
// サービスの開始
///////////////////////////////////////////////////////////////////////////////
bool CService::run()
{
	bool bRet;

	// サービスメイン関数をSCMに登録する
	bRet = registServiceMain();
	if (!bRet) {
		logObj.log("registServiceMain error.");
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// SCMにサービスのmain関数を登録する
///////////////////////////////////////////////////////////////////////////////
bool CService::registServiceMain()
{
	SERVICE_TABLE_ENTRY	serviceTable[] =
	{
		{SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)(ServiceMain)},
		{NULL,NULL}
	};

	logObj.log("%s started.", __FUNCTION__);

	// SCMに登録する
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
// SCMにサービスのmain関数を登録する
///////////////////////////////////////////////////////////////////////////////
VOID CService::ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	//logObj.log("%s started.", __FUNCTION__);

#if 0
	// 即座に登録関数を呼び出す
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
		(LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		logObj.log("RegisterServiceCtrlHandler error.", GetLastError());
		terminate(GetLastError());
		return;
	}

	logObj.log("RegisterServiceCtrlHandler normal ended.");

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		logObj.log("SendStatusToSCM error.");
		terminate(GetLastError());
		return;
	}

	// 終了イベントを作成する
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);
	if (!terminateEvent)
	{
		logObj.log("CreateEvent error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}
	logObj.log("CreateEvent normal ended");

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		logObj.log("SendStatusToSCM(2) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 開始パラメータの有無をチェックする
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

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		logObj.log("SendStatusToSCM(3) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// サービス自体を開始する
	success = InitService();
	if (!success)
	{
		logObj.log("InitService error");
		terminate(GetLastError());
		return;
	}

	// この時点でサービスは実行状態になっている。
	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		logObj.log("SendStatusToSCM(4) error. %d", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 終了シグナルを待機し、それを検出したら終了する。
	logObj.log("Before WaitForSingleObject");
	WaitForSingleObject(terminateEvent, INFINITE);
	logObj.log("After WaitForSingleObject");

	terminate(0);

	logObj.log("%s ended.", __FUNCTION__);
#endif
	return;
}
