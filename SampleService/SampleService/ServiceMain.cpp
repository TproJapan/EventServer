///////////////////////////////////////////////////////////////////////////////
// サービスプログラムのサンプル
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
// 外部変数
///////////////////////////////////////////////////////////////////////////////
CLog logObj;
SERVICE_STATUS_HANDLE serviceStatusHandle;
						// 状態情報をSCMとの間で通知しあうために使用するハンドル
						// RegisterServiceCtrlHandler関数によって作成される

// ServiceMainが完了するのを防止するために使用するイベント
HANDLE terminateEvent = NULL;

// 実際の処理を行うためのスレッド
HANDLE threadHandle = 0;

// サービスの現在の状態を格納するフラグ
BOOL pauseServise = FALSE;
BOOL runningService = FALSE;

///////////////////////////////////////////////////////////////////////////////
// プロトタイプ宣言
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
// メイン処理
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	bool bRet;
	
	// ログファイルを作成
	bRet = logObj.open(LOGFILE_NAME);
	logObj.log("main started");

	// サービスメイン関数をSCMに登録する
	bRet = registServiceMain();
	if (!bRet) {
		logObj.log("registServiceMain error.");
		return -1;
	}

	logObj.log("main ended");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// SCMにサービスのmain関数を登録する
///////////////////////////////////////////////////////////////////////////////
bool registServiceMain()
{
	SERVICE_TABLE_ENTRY	serviceTable[] =
	{
		{SERVICE_NAME,(LPSERVICE_MAIN_FUNCTION)ServiceMain},
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
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	logObj.log("%s started.", __FUNCTION__);

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
	return;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain関数でエラーが発生した場合の処理として、
// クリーンアップを行い、サービスが開始しなかったことを
// SCMに通知する。
///////////////////////////////////////////////////////////////////////////////
VOID terminate(DWORD error)
{
	logObj.log("%s started.", __FUNCTION__);

	// terminateEventハンドルが作成されている場合は閉じる
	if (terminateEvent) {
		logObj.log("terminateEventをクローズ.");
		CloseHandle(terminateEvent);
	}

	// サービスが停止したことを通知するためにSCMにメッセージを送信する
	if (serviceStatusHandle)
	{
		logObj.log("SCMにサービスの停止を通知.");
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// スレッドが開始されている場合は、それを終了する
	if (threadHandle)
	{
		logObj.log("threadHandleをクローズ.");
		CloseHandle(threadHandle);
	}

	// serviceStatusHandleは閉じる必要がない。

	logObj.log("%s ended.", __FUNCTION__);
	return;
}

///////////////////////////////////////////////////////////////////////////////
// この関数は、SetServiceStatus関数によって
// サービスの状態を更新する処理をまとめて実行する。
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

	// SERVICE_STATUS構造体のすべてのメンバに値を設定する。
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = dwCurrentState;

	// 何らかの処理を行っている場合は、コントロールイベントを受け取らない。
	// それ以外の場合は、コントロールイベントを受け取る。
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

	// 特定の終了コードが定義されている場合は
	// win32の終了コードを正しく設定する。
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

	// 状態レコードをSCMに渡す。
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
// サービスコントロールマネージャから受け取ったイベントを
// ディスパッチする。
///////////////////////////////////////////////////////////////////////////////
VOID ServiceCtrlHandler(DWORD controlCode)
{
	DWORD currentState = 0;
	BOOL success;

	logObj.log("%s started.", __FUNCTION__);

	switch (controlCode)
	{
		// 開始時はSeviceMain関数が呼び出されるので
		// START(開始)オプションはない

		// サービスを停止する
	case SERVICE_CONTROL_STOP:
		logObj.log("SERVICE_CONTROL_STOP");
		currentState = SERVICE_STOP_PENDING;
		// 現在の状態をSCMに通知する
		success = SendStatusToSCM(SERVICE_STOP_PENDING,
									NO_ERROR, 0, 1, 5000);
		// 成功しなかった場合、特に何もしない

		// サービスを停止する
		StopService();
		return;

		// サービスを一時停止する
	case SERVICE_CONTROL_PAUSE:
		logObj.log("SERVICE_CONTROL_PAUSE");
		if (runningService && !pauseServise)
		{
			// 現在の状態をSCMに通知する
			success = SendStatusToSCM(SERVICE_PAUSE_PENDING,
										NO_ERROR, 0, 1, 1000);
			PauseService();
			currentState = SERVICE_PAUSED;
		}
		break;

		// 一時停止から再開する
	case SERVICE_CONTROL_CONTINUE:
		logObj.log("SERVICE_CONTROL_CONTINUE");
		if (runningService && pauseServise)
		{
			// 現在の状態をSCMに通知する
			success = SendStatusToSCM(SERVICE_CONTINUE_PENDING,
										NO_ERROR, 0, 1, 1000);
			ResumeService();
			currentState = SERVICE_RUNNING;
		}
		break;

		// 現在の状態を更新する
	case SERVICE_CONTROL_INTERROGATE:
		// このswitch分の後に行に進み、状態を送信する
		logObj.log("SERVICE_CONTROL_INTERROGATE");
		break;

		// シャットダウン時には何もしない。ここでクリーンアップを
		// 行うことができるが、非常にすばやく行わなければならない。
	case SERVICE_CONTROL_SHUTDOWN:
		// シャットダウン時には何もしない。
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
// サービスを初期化するためにそのスレッドを開始する。
///////////////////////////////////////////////////////////////////////////////
BOOL InitService()
{
	DWORD id;

	logObj.log("%s started.", __FUNCTION__);

	// サービスのスレッドを開始する
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
// ServiceMain関数を完了させることによって、サービスを停止する。
///////////////////////////////////////////////////////////////////////////////
VOID StopService()
{
	logObj.log("%s started.", __FUNCTION__);

	runningService = FALSE;

	//Tcpサーバ停止関数呼び出し
	StopTcpServer();
	logObj.log("Tcpサーバ停止を開始しました");

	// ServiceMain関数が制御を返すことができるようにするために
	// ServiceMain関数が完了するのを防止しているイベントをセットする。
	SetEvent(terminateEvent);

	logObj.log("%s ended.", __FUNCTION__);
	return;
}

///////////////////////////////////////////////////////////////////////////////
// 一時停止されたサービスを再開する
///////////////////////////////////////////////////////////////////////////////
VOID ResumeService()
{
	logObj.log("%s started.", __FUNCTION__);
	pauseServise = FALSE;
	ResumeThread(threadHandle);
	logObj.log("%s ended.", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////
// サービスを一時停止する
///////////////////////////////////////////////////////////////////////////////
VOID PauseService()
{
	logObj.log("%s started.", __FUNCTION__);
	pauseServise = TRUE;
	SuspendThread(threadHandle);
	logObj.log("%s ended.", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////////////
// サービス本来の処理
///////////////////////////////////////////////////////////////////////////////
DWORD ServiceThread(LPDWORD param)
{
	logObj.log("%s started.", __FUNCTION__);

//	while (1)
//	{
//		logObj.log("サービス、サービス!!!");
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
