#include <windows.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	//引数チェック
	if (argc != 2) {
		printf("Usage : Stop Message\n");
		return -1;
	}

	HANDLE hPipe;
	hPipe = 
		CreateNamedPipe("\\\\.\\pipe\\EventServer",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_WAIT,
		1, 0, 0, 150, (LPSECURITY_ATTRIBUTES)NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}


	return 0;
}