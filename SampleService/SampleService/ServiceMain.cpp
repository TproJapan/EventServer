#ifdef _WIN64
///////////////////////////////////////////////////////////////////////////////
// サービスプログラムのサンプル
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "../src/BoostLog.h"
#include "../src/TcpServer.h"
#include <Windows.h>
#define BUF 1024
#define PortNumTxtPath "..\\..\\SampleService\\PortNum.txt"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
const char* LOGFILE_NAME = "C:\\tmp\\SampleService.log";
LPWSTR SERVICE_NAME = (LPWSTR)L"SampleService";

///////////////////////////////////////////////////////////////////////////////
// 外部変数
///////////////////////////////////////////////////////////////////////////////
//CLog logObj;
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
TcpServer* tcpServer;
///////////////////////////////////////////////////////////////////////////////
// メイン処理
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
	logging::add_common_attributes();

	bool bRet;

	write_log(2, "main started, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	// サービスメイン関数をSCMに登録する
	bRet = registServiceMain();
	if (!bRet) {
		write_log(4, "registServiceMain error. (%ld),%s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		return -1;
	}

	write_log(2, "main ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
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

	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// SCMに登録する
	BOOL success = StartServiceCtrlDispatcher(serviceTable);
	if (!success)
	{
		write_log(4, "StartServiceCtrlDispatcher error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return false;
	}

	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SCMにサービスのmain関数を登録する
///////////////////////////////////////////////////////////////////////////////
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// 即座に登録関数を呼び出す
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
		(LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		write_log(4, "RegisterServiceCtrlHandler error. %s %d %s\n", __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	write_log(2, "RegisterServiceCtrlHandler normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		write_log(2, "SendStatusToSCM error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		terminate(GetLastError());
		return;
	}

	// 終了イベントを作成する
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);
	if (!terminateEvent)
	{
		write_log(4, "CreateEvent error. %d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	write_log(2, "CreateEvent normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		write_log(4, "SendStatusToSCM(2) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		write_log(4, "SendStatusToSCM(3) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	// サービス自体を開始する
	success = InitService();
	if (!success)
	{
		write_log(4, "InitService error. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		terminate(GetLastError());
		return;
	}

	// この時点でサービスは実行状態になっている。
	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		write_log(4, "SendStatusToSCM(4) error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	WaitForSingleObject(terminateEvent, INFINITE);
	write_log(2, "After WaitForSingleObject. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	terminate(0);

	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain関数でエラーが発生した場合の処理として、
// クリーンアップを行い、サービスが開始しなかったことを
// SCMに通知する。
///////////////////////////////////////////////////////////////////////////////
VOID terminate(DWORD error)
{
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// terminateEventハンドルが作成されている場合は閉じる
	if (terminateEvent) {
		write_log(2, "terminateEventをクローズ. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		CloseHandle(terminateEvent);
	}

	// サービスが停止したことを通知するためにSCMにメッセージを送信する
	if (serviceStatusHandle)
	{
		write_log(2, "SCMにサービスの停止を通知. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// スレッドが開始されている場合は、それを終了する
	if (threadHandle)
	{
		write_log(2, "threadHandleをクローズ. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		CloseHandle(threadHandle);
	}

	// serviceStatusHandleは閉じる必要がない。

	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

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

	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

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
		write_log(4, "SetServiceStatus error. %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		StopService();
	}

	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

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

	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	switch (controlCode)
	{
		// 開始時はSeviceMain関数が呼び出されるので
		// START(開始)オプションはない

		// サービスを停止する
	case SERVICE_CONTROL_STOP:
		write_log(2, "SERVICE_CONTROL_STOP. %s %d %s\n", __FILENAME__, __LINE__, __func__);

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
		write_log(2, "SERVICE_CONTROL_PAUSE. %s %d %s\n", __FILENAME__, __LINE__, __func__);

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
		write_log(2, "SERVICE_CONTROL_CONTINUE. %s %d %s\n", __FILENAME__, __LINE__, __func__);

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
		write_log(2, "SERVICE_CONTROL_INTERROGATE. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		break;

		// シャットダウン時には何もしない。ここでクリーンアップを
		// 行うことができるが、非常にすばやく行わなければならない。
	case SERVICE_CONTROL_SHUTDOWN:
		// シャットダウン時には何もしない。
		write_log(2, "SERVICE_CONTROL_SHUTDOWN. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return;

	default:
		write_log(2, "default. %s %d %s\n", __FILENAME__, __LINE__, __func__);
		break;
	}

	success = SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// サービスを初期化するためにそのスレッドを開始する。
///////////////////////////////////////////////////////////////////////////////
BOOL InitService()
{
	DWORD id;
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	// サービスのスレッドを開始する
	threadHandle = CreateThread(0, 0,
		(LPTHREAD_START_ROUTINE)ServiceThread,
		0, 0, &id);
	if (threadHandle == 0)
	{
		write_log(4, "CreateThread error. %d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		return FALSE;
	}

	write_log(2, "CreateThread normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
	runningService = TRUE;
	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// ServiceMain関数を完了させることによって、サービスを停止する。
///////////////////////////////////////////////////////////////////////////////
VOID StopService()
{
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	runningService = FALSE;
	write_log(2, "TcpServerMainEndの待機を開始しました. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	// 現在の状態をSCMに通知する
	BOOL success = SendStatusToSCM(SERVICE_STOP_PENDING,
		NO_ERROR, 0, 1, 30000);

	//WaitForSingleObject(TcpServerMainEnd,INFINITE);
#if 1
	TcpServerMainEnd = CreateEvent(0, TRUE, FALSE, 0);
	if (!TcpServerMainEnd)
	{
		write_log(4, "CreateEvent error.  %d %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

		terminate(GetLastError());
		return;
	}

	write_log(2, "CreateEvent normal ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);

	//Tcpサーバ停止関数呼び出し
	write_log(2, "Tcpサーバ停止を開始しました. %s %d %s\n", __FILENAME__, __LINE__, __func__);

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

	write_log(2, "strRet = %s. %s %d %s\n", strRet.c_str(), __FILENAME__, __LINE__, __func__);

	if (dwRet == WAIT_FAILED) {
		write_log(4, "WaitForSingleObject error.code=%d. %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);

	}
#endif

	write_log(2, "TcpServerMainEndの待機を終了しました. %s %d %s\n", __FILENAME__, __LINE__, __func__);

#if 1
	if (TcpServerMainEnd)
	{
		CloseHandle(TcpServerMainEnd);
		write_log(2, "CloseHandle ended. %s %d %s\n", __FILENAME__, __LINE__, __func__);
	}
#endif

	// ServiceMain関数が完了するイベントをセットする。
	SetEvent(terminateEvent);
	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	return;
}

///////////////////////////////////////////////////////////////////////////////
// 一時停止されたサービスを再開する
///////////////////////////////////////////////////////////////////////////////
VOID ResumeService()
{
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	pauseServise = FALSE;
	ResumeThread(threadHandle);
	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// サービスを一時停止する
///////////////////////////////////////////////////////////////////////////////
VOID PauseService()
{
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	pauseServise = TRUE;
	SuspendThread(threadHandle);
	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// サービス本来の処理
///////////////////////////////////////////////////////////////////////////////
DWORD ServiceThread(LPDWORD param)
{
	write_log(2, "%s started. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);

	write_log(2, "Before GetModuleFileName executed! %s %d %s\n", __FILENAME__, __LINE__, __func__);
	TCHAR path[MAX_PATH];
	if (::GetModuleFileName(NULL, path, MAX_PATH))    //実行ファイルのフルパスを取得
	{   //取得に成功
		write_log(2, "Current Path is %s %s %d %s\n", path, __FILENAME__, __LINE__, __func__);
	}
	else
	{
		write_log(2, "Failed to get FilePath %s %d %s\n", __FILENAME__, __LINE__, __func__);
	}

	try {
		auto src = fopen(PortNumTxtPath, "r");

		if (src == NULL) {
			write_log(2, "%s not found!  %s %d %s\n", "PortNum.txt", __FILENAME__, __LINE__, __func__);
			return -1;
		}

		int i = 0;
		char port[BUF] = "";
		while (fgets(port, BUF, src) != NULL) {
			if (strlen(port) >= 2
				&& port[strlen(port) - 1] == '\n'
				&& port[strlen(port) - 2] == '\r')
			{
				port[strlen(port) - 2] = '\0';
			}

			if (strlen(port) >= 1
				&& port[strlen(port) - 1] == '\n')
			{
				port[strlen(port) - 1] = '\0';
			}

			i++;

			if (i >= 1) break;
		}

		if (strcmp(port, "") == 0)
		{
			write_log(2, "Port Number does not specified first line in %s!  %s %d %s\n", "PortNum.txt", __FILENAME__, __LINE__, __func__);
			return -1;
		}

		int PortNum = atoi(port);
		if (PortNum == 0)
		{
			write_log(2, "Port Number atoi failed!  %s %d %s\n", __FILENAME__, __LINE__, __func__);
			return -1;
		}
		else {
			write_log(2, "Port Number is %d %s %d %s\n", PortNum, __FILENAME__, __LINE__, __func__);
		}

		tcpServer = new TcpServer(PortNum);
	}
	catch (int e) {
		return -1;
	}

	int result = tcpServer->Func();

	if (result != 0)
	{
		write_log(4, "Tcpserver() ended with error %d! %s %d %s\n", result, __FILENAME__, __LINE__, __func__);
	}


	write_log(4, "Before Destructor run %d! %s %d %s\n", result, __FILENAME__, __LINE__, __func__);
	delete tcpServer;
	write_log(4, "After Destructor run %d! %s %d %s\n", result, __FILENAME__, __LINE__, __func__);

	write_log(2, "%s ended. %s %d %s\n", __func__, __FILENAME__, __LINE__, __func__);
	return 0;
}
#endif