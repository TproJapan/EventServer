#include <windows.h>
#include <stdio.h>

int duration = 10000;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
const char* passbuf = "C:\\Users\\suguru.tateda\\VSProjects\\EventServer\\EventServer\\x64\\Debug\\EventServer2.exe 5000";
//const char* passbuf = "C:\\VS2019Projects\\tcp_thread_nodaemon_text_win\\EventServer\\Debug\\EventServer2.exe 5000 > NUL"; // konishi
//const char* passbuf = "C:\\Users\\zx9y-\\source\\repos\\EventServer-tcp_thread_nodaemon_text_win-3\\EventServer\\x64\\Debug\\EventServer2.exe 64101";
//const char* passbuf = "C:\\Users\\zx9y-\\source\\repos\\EventServer-tcp_thread_nodaemon_text_win-3\\EventServer\\x64\\Debug\\EventServer2.exe 64101 > NUL";
//const char* passbuf = "C:\\Users\\zx9y-\\source\\repos\\EventServer-tcp_thread_nodaemon_text_win-3\\EventServer\\x64\\Debug\\EventServer2.exe 64101";

char pipeName[80];
const char* MESSAGE = "Are you OK?";
HANDLE hPipe;
FILE* fp;

int CheckAlive();
BOOL CreateEventServer(PROCESS_INFORMATION& processInfo);
BOOL BCreateProcess = FALSE;	// konishi
PROCESS_INFORMATION	pcocessInfo;// konishi

int main()
{
	// コンピューター名の取得	
	char machineName[80];
	DWORD bufflen;
	GetComputerName((LPSTR)machineName, &bufflen);

	// パイプ名の組み立て
	//wsprintf(pipeName, PIPE_NAME, machineName);
	wsprintf(pipeName, PIPE_NAME, machineName);

	//定期チェック
	while (true) {
		Sleep(duration);

		if (CheckAlive() == -1) {
			printf("Before System.\n");

			//int result = system(passbuf);
/*
#if 1
			// konishi-start
			if (fp != NULL) {
				printf("Before _pclose\n");
				int nRet = _pclose(fp);
				printf("_pclose nRet=%d\n", nRet);
				fp = NULL;
			}
			// konishi-end
#endif
			//標準入出力を閉じる
			fp = _popen(passbuf, "r");
			if (fp == NULL) {
				printf("System error.(%ld)\n", GetLastError());
				break;
			}
*/
//前回チェックでプロセス生成したのに、今回Pipe応答がなかった場合という事?
//前回チェックでプロセス生成したのに、今回Pipeが無い場合という事?
			if (BCreateProcess) {
				printf("Before WaitForSingleObject.\n");
				WaitForSingleObject(pcocessInfo.hProcess, INFINITE);//これは何をしているのか?
				//プロセス情報を破棄する関数を呼ぶ??
				//CloseHandle(pcocessInfo.hProcess);??
				printf("After WaitForSingleObject.\n");
			}

			BCreateProcess = CreateEventServer(pcocessInfo);
			printf("After System.\n");
			continue;
		}

		// konishi-start
#if 0
		printf("Before fgets\n");
		if (fp != NULL) {
			char buf[32];
			//while (!feof(fp)) {
			fgets(buf, sizeof(buf), fp);
			printf("=> %s\n", buf);
			//}
		}
#endif
		// konishi-end


	}

	//if(fp != 0) _pclose(fp);
	printf("Observerを終了\n");
	return 0;
}

int CheckAlive() {
	// サーバのパイプに接続
#if 0 // konishi0516
	hPipe = CreateFile(pipeName,
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
		(LPSECURITY_ATTRIBUTES)NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);
#else // konishi0516
	hPipe = CreateFile(pipeName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		(LPSECURITY_ATTRIBUTES)NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		(HANDLE)NULL);
#endif // konishi0516

	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("Observer failed to connect NamedPipe(%ld)\n", GetLastError());
		return -1;//パイプが存在しない場合という事?
	}
	printf("Observer CreateFile normal ended\n");// konishi

	//　データの送信
	BOOL bRet;
	DWORD NumBytesWritten;
	bRet = WriteFile(hPipe,
		MESSAGE,
		(DWORD)strlen(MESSAGE) + 1,
		&NumBytesWritten,
		(LPOVERLAPPED)NULL);

	if (bRet != TRUE) {
		printf("WriteFile error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
		return -1;
	}
	printf("Observer WriteFile normal ended\n");// konishi

	///////////////////////////////////////////////////////////
	// OVARLAPPED構造体の初期設定
	OVERLAPPED overlappedRead;
	memset(&overlappedRead, 0, sizeof(overlappedRead));
	HANDLE eventRead = CreateEvent(0, FALSE, FALSE, 0);
	if (eventRead == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}
	overlappedRead.hEvent = eventRead;

	DWORD NumBytesRead;
	char buf[1024];
	bRet = ReadFile(hPipe,
		buf,
		sizeof(buf),
		&NumBytesRead,
		(LPOVERLAPPED)&overlappedRead);
	// エラーでも「重複I/O 処理を実行中」の場合は正常扱い
	if (bRet != TRUE &&
		GetLastError() != ERROR_IO_PENDING)
	{
		printf("ReadFile error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
		return -1;
	}

	printf("ReadFile nRet=%ld, GetLastError=%ld\n", bRet, GetLastError());

	printf("** ERROR_IO_PENDING = %ld\n", ERROR_IO_PENDING);
	printf("** WAIT_FAILED = %ld\n", WAIT_FAILED);
	printf("** WAIT_TIMEOUT = %ld\n", WAIT_TIMEOUT);


	///////////////////////////////////////////////////////////
	// サーバからの応答を受信(ReadFileのハンドラをWaitForMultipleEvent(WaitForSingleObjectでもいい?)
	// に渡してタイムアウト設定しないと無限待ちが発生する可能性がある)

	//DWORD nRet = WaitForSingleObject(hPipe, (DWORD)duration);

	while (true) {
		DWORD nRet = WaitForSingleObject(eventRead, (DWORD)duration);
		printf("WaitForSingleObject nRet=%ld\n", nRet);
		if (nRet == WAIT_FAILED)
		{
			printf("WaitForSingleObject error. (%ld)\n", GetLastError());
			return -1;
		}

		if (nRet == WAIT_TIMEOUT) {
			//printf("タイムアウト発生!!!\n");
			printf("timeout error!!!\n");
			continue;
		}

		break;
	}

	//printf("WaitForSingleObject nRet=%ld\n", nRet);

	//////
	DWORD byteTransfer;
	bRet = GetOverlappedResult(eventRead, &overlappedRead, &byteTransfer, TRUE);
	printf("GetOverlappedResult bRet = %ld\n", bRet);
	if (bRet != TRUE) {
		printf("GetOverlappedResult error\n");
		//DisconnectNamedPipe(hPipe);
		return -1;
	}
	ResetEvent(eventRead);
	//////



#if 0 // konishi0516
	//パイプメッセージ受信処理
	DWORD NumBytesRead;
	char buf[1024];
	bRet = ReadFile(hPipe,
		buf,
		sizeof(buf),
		&NumBytesRead,
		(LPOVERLAPPED)NULL);

	if (bRet != TRUE) {
		printf("ReadFile error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
		return -1;
	}
#endif // konmishi0516

	static int count = 0;	// konishi
	//printf("サーバからの応答 : [%s]\n", buf);
	//printf("サーバからの応答(%d) : [%s]\n", count++, buf);
	printf("Response from sever(%d) : [%s]\n", count++, buf);
	CloseHandle(hPipe);
	return 0;
}

BOOL CreateEventServer(PROCESS_INFORMATION& processInfo)
{
	BOOL ret;
	STARTUPINFO	startupInfo;
	//PROCESS_INFORMATION	pcocessInfo;

	GetStartupInfo(&startupInfo);//初期化

	ret = CreateProcess(NULL,
		(LPSTR)passbuf,
		NULL, NULL, FALSE,
		CREATE_NEW_CONSOLE,
		NULL, NULL,
		&startupInfo,
		&processInfo);
	if (ret == FALSE) {
		printf("CreateProcess error. (%d)\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}