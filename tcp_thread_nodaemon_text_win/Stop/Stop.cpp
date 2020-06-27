#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef _WIN32
#else
#include <unistd.h>
#include <err.h>
#endif

int main(int argc, char* argv[])
{
	//引数チェック
	if (argc != 2) {
		perror("Stop Process No\n");
		return -1;
	}

	//ps aux | grep -e TcpServerで調べたプロセスidにSIGUSR1を送る
	int target_pid = atoi(argv[1]);
#ifdef _WIN32
#else
	kill(target_pid, SIGUSR2);
#endif
	return 0;
}