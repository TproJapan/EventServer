#include <windows.h>
#include <stdio.h>

int duration = 10000;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
const char* passbuf = "C:\\VS2019Projects\\tcp_thread_nodaemon_text_win\\EventServer\\Debug\\EventServer2.exe 5000";
char pipeName[80];
const char* MESSAGE = "Are you OK?";
HANDLE hPipe;
FILE* fp;

int CheckAlive();

int main()
{
	// コンピューター名の取得	
	char machineName[80];
	DWORD bufflen;
	GetComputerName((LPSTR)machineName, &bufflen);

	// パイプ名の組み立て
	wsprintf(pipeName, PIPE_NAME, machineName);

	//定期チェック
    while (true) {
        Sleep(duration);

		if (CheckAlive() == -1) {
			printf("Before System.\n");

			//int result = system(passbuf);

			//標準入出力を閉じる
			fp = _popen(passbuf, "r");
			if (fp == NULL) {
				printf("System error.(%ld)\n", GetLastError());
				break;
			}

			printf("After System.\n");
			continue;
		}
    }

	if(fp != 0) _pclose(fp);
	printf("Observerを終了\n");
	return 0;
}

int CheckAlive() {
	// サーバのパイプに接続
	hPipe = CreateFile(pipeName,
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
		(LPSECURITY_ATTRIBUTES)NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);

	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("Observer failed to connect NamedPipe(%ld)\n", GetLastError());
		return -1;
	}

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

	// サーバからの応答を受信
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

	printf("サーバからの応答 : [%s]\n", buf);
	CloseHandle(hPipe);
	return 0;
}