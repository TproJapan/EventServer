//#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>

using namespace std;
// グローバル変数

// サービスの名前
//char* SERVICE_NAME = (char*)"BeepService";
char* SERVICE_NAME = (char*)"beep_srv";
//#define SERVICE_NAME L"BeepService"

// ServiceMainが完了するのを防止するために使用するイベントハンドル
// Handleとは構造体へのポインタ。メンバ変数に値を書き込みに行く人と、イベントが発生する。
// Handle変数領域のどこかにイベント番号をセットする領域がある。これによってイベントとの結び付けができる
// イベントはWaitForSingleEventとかWaitForMultiEventで待つ。
// イベントが発生する or ハンドラがクローズされるとWaitが解ける
// イベントはプログラム上は「数値」で扱われる
// ハンドラはイベントが起きてから行われる処理内容。関数名
HANDLE terminateEvent = NULL;


// 状態情報をSCMとの間で通知しあうために使用するハンドル
// RegisterServiceCtrlHandler関数によって作成される
SERVICE_STATUS_HANDLE serviceStatusHandle;

// ビープ音を鳴らすミリ秒単位の間隔
int beepDelay = 2000;

// サービスの現在の状態を格納するフラグ
BOOL pauseServise = FALSE;
BOOL runningService = FALSE;

// 実際の処理を行うためのスレッドハンドル
// CreateThread関数の戻り値がHandle型のスレッドハンドル
// Unixだと戻り値はスレッドid
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
	cout << "エラー番号: " << err << endl;
	ExitProcess(err);

}
//下の方でLPTHREAD_START_ROUTINE型の関数ポインタを要求されているので自作する
//LPTHREAD_START_ROUTINE型というのは引数がLPDWORD型、戻り値がDWORDである必要がある
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
		//イベント待ち
		nRet = WSAWaitForMultipleEvents(1, &hEvent, FALSE, WSA_INFINITE, FALSE);

		if (nRet == WSA_WAIT_FAILED)
		{
			log("!!!!!!!!!!!!!!!!!!!!",0);
			break;
		}

		//イベントの調査
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

	WSACloseEvent(hEvent);//イベントクローズ

	return 0;
}

// サービスを初期化するためにそのスレッドを開始する。
BOOL InitService()
{
	struct sockaddr_in	addr;
	int	addrlen;
	//HANDLE	hThread;
	DWORD	threadId;
	WORD	wVersionRequested = MAKEWORD(2, 2);
	WSADATA	wsaData;

	WSAStartup(wVersionRequested, &wsaData);	//Winsock初期化

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		log("socket", GetLastError());
		return -1;
	}

	//ポートの設定
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

	// サービスのスレッドを開始する
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

// 一時停止されたサービスを再開する
VOID ResumeService()
{
	pauseServise = FALSE;
	ResumeThread(threadHandle);
}

// サービスを一時停止する
VOID PauseService()
{
	pauseServise = TRUE;
	SuspendThread(threadHandle);
}

// ServiceMain関数を完了させることによって、サービスを
// 停止する。
VOID StopService()
{
	runningService = FALSE;

	// ServiceMain関数が制御を返すことができるようにするために
	// ServiceMain関数が完了するのを防止しているイベントをセットする。
	SetEvent(terminateEvent);
}

// この関数は、SetServiceStatus関数によって
// サービスの状態を更新する処理をまとめて実行する。
BOOL SendStatusToSCM(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint)
{
	BOOL success;
	SERVICE_STATUS serviceStatus;

	// SERVICE_STATUS構造体のすべてのメンバに値を設定する。
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = dwCurrentState;

	// 何らかの処理を行っている場合は、コントロールイベントを受け取らない。
	// それ以外の場合は、コントロールイベントを受け取る。
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

	// 特定の終了コードが定義されている場合は
	// win32の終了コードを正しく設定する。
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

	// 状態レコードをSCMに渡す。
	success = SetServiceStatus(serviceStatusHandle, &serviceStatus);

	if (!success)
	{
		StopService();
	}

	return success;
}

// サービスコントロールマネージャから受け取ったイベントを
// ディスパッチする。
VOID ServiceCtrlHandler(DWORD controlCode)
{
	DWORD currentState = 0;
	BOOL success;

	switch (controlCode)
	{
		// 開始時はSeviceMain関数が呼び出されるので
		// START(開始)オプションはない

		// サービスを停止する
	case SERVICE_CONTROL_STOP:
		log("SERVICE_CONTROL_STOP", 0);
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
		log("SERVICE_CONTROL_PAUSE", 0);
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
		log("SERVICE_CONTROL_CONTINUE", 0);
		if (runningService && !pauseServise)
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
		break;

		// シャットダウン時には何もしない。ここでクリーンアップを
		// 行うことができるが、非常にすばやく行わなければならない。
		// 長大な処理はシャットダウンまでの猶予に間に合わずOSに殺される
	case SERVICE_CONTROL_SHUTDOWN:
		// シャットダウン時には何もしない。
		return;

	default:
		break;
	}

	SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
}


// ServiceMain関数でエラーが発生した場合の処理として、
// クリーンアップを行い、サービスが開始しなかったことを
// SCMに通知する。クリーンアップとは動的に取得したリソースを開放したり接続を切ったりする事・
//今回の場合はterminateハンドルを閉じてbeepのワーカースレッドを閉じる事。
VOID terminate(DWORD error)
{
	// terminateEventハンドルが作成されている場合は、それを閉じる
	if (terminateEvent) {
		CloseHandle(terminateEvent);
	}

	// サービスが停止したことを通知するために
	// SCMにメッセージを送信する
	if (serviceStatusHandle)
	{
		SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}

	// スレッドが開始されている場合は、それを終了する
	if (threadHandle)
	{
		CloseHandle(threadHandle);

	}
	// serviceStatusHandleは閉じる必要がない。
}

// ServiceMain関数は、SCMがサービスを開始する時に呼び出される。
// ServiceMain関数がreturn(制御を返す)と、サービスは停止する。
// したがって関数の実行が終了する直前まで、ServiceMain関数は
// イベントを待機している状態にしておき、停止する時になったら
// そのイベントをシグナル状態にセットする。(シグナル状態:イベントが発生した瞬間の事)
// またエラーが発生した場合にはサービスを開始できないので、
// エラーが発生した場合にもServiceMain関数はreturn(制御を返す)。
VOID ServiceMain(DWORD argc, LPSTR* argv)
{
	BOOL success;

	// 即座に登録関数を呼び出す
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

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
	if (!success)
	{
		log("SendStatusToSCM(1) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 終了イベントを作成する
	log("Before CreateEvent", 0);
	terminateEvent = CreateEvent(0, TRUE, FALSE, 0);//第一:セキュリティ第二:手動リセット第三:初期状態第四イベントの名前(文字列へのポインタ)
	//本来の使い方はSetEvent(イベント名)で第三引数がTrueになってイベントをemitする
	if (!terminateEvent)
	{
		log("CreateEvent error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 2, 1000);
	if (!success)
	{
		log("SendStatusToSCM(2) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 開始パラメータの有無をチェックする
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

	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
	if (!success)
	{
		log("SendStatusToSCM(3) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// サービス自体を開始する
	success = InitService();
	if (!success)
	{
		log("InitService error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// この時点でサービスは実行状態になっている。
	// 進行状況をSCMに通知する
	success = SendStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 0, 0);
	if (!success)
	{
		log("SendStatusToSCM(4) error", GetLastError());
		terminate(GetLastError());
		return;
	}

	// 終了シグナルを待機し、それを検出したら終了する。
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

	// SCMに登録する
	log("check-01", 0);
	success = StartServiceCtrlDispatcher(serviceTable);
	if (!success)
	{
		log("StartServiceCtrlDispatcherでエラーが発生", GetLastError());
		ErrorHandler((char*)"StartServiceCtrlDispatcherでエラーが発生", GetLastError());
	}

	return 0;
}