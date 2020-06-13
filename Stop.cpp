#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
 
int main(int argc, char *argv[])
{
	//引数チェック
	if ( argc != 2 ) {
		perror("Stop Process No\n");
		return -1;
	} 

    //ps aux | grep -e TcpServerで調べたプロセスidにSIGUSR1を送る
    int target_pid = atoi(argv[1]);
    kill(target_pid, SIGUSR2);

    return 0;
}