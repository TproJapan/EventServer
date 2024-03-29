﻿#ifdef __GNUC__
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
		//perror("Stop Process No\n");
		printf("%ld:%s", errno, "Stop Process No\n");
		return -1;
	} 

    //ps aux | grep -e TcpServerで調べたプロセスidにSIGUSR2を送る
    int targetPid = atoi(argv[1]);
    kill(targetPid, SIGUSR2);

    return 0;
}
#endif