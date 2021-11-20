///////////////////////////////////////////////////////////////////////////////
// [stop]
//  機能 : TcpServerに名前付きパイプて゜メッセージを送信する。
//  構文 : stop 送信メッセージ
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//引数チェック
	if (argc != 2) {
		printf("Usage : Stop Message\n");
		return -1;
	}

	// コンピューター名の取得	
	char machineName[80];
	DWORD bufflen;
	GetComputerName((LPSTR)machineName, &bufflen);

	// パイプ名の組み立て
	char pipeName[80];
	wsprintf(pipeName, PIPE_NAME, machineName);

	// サーバのパイプに接続
	HANDLE hPipe = CreateFile(pipeName,
							GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
							(LPSECURITY_ATTRIBUTES)NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							(HANDLE)NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	//　データの送信
	BOOL bRet;
	DWORD NumBytesWritten;
	bRet = WriteFile(hPipe,
						argv[1],
						(DWORD)strlen(argv[1]) + 1,
						&NumBytesWritten,
						(LPOVERLAPPED)NULL);
	if ( bRet != TRUE ) {
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

	bRet = CloseHandle(hPipe);
	if (bRet != TRUE) {
		printf("CloseHandle error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
	}

	return 0;
}