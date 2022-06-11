#ifdef __GNUC__
///////////////////////////////////////////////////////////////////////////////
//  Start Command
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <getopt.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include "TcpCommon.h"
using namespace std;

int fd_start = -1;//立ち上がり完了報告用fd;
int nbyte;
char buff[256];
int nRet;

int closeStartPipe();
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//引数チェック
	// GetServByName
	if ( argc == 1 || argc > 3 ) {
		perror("Start Option error\n");
		printf("E.G: # ./Start 5000 or # ./Start -f 5000\n");
		return -1;
	} 

	// 入力パイプの作成
	unlink(PIPE_START);
	nRet = mkfifo(PIPE_START, 0666);
	if ( nRet ==-1 ) {
		perror("mkfifo\n");
		return -1;
	}
	
	//エラー判定値(int)
	int nRet = 0;

	////////////////////////
	//TcpServerプロセス起動
	////////////////////////

	//StartコマンドプロセスID取得
	pid_t r_pid = getpid();
	char pid_buff[16];
    char port_buff[16];

	strcpy(port_buff, argv[1]);
    sprintf(pid_buff, "%d", r_pid);//文字列に変換

	char TcpServer_path[256];
	sprintf(TcpServer_path, "%s%s",PROJ_HOME, "/ServerMain");
	char* const str[] = {(char*)"myServer", port_buff, pid_buff, NULL};
	pid_t pid = 0;
	
	pid = fork();

    if (pid == -1 ) 
	{
        printf("fork has failed in Start.cpp\n");
		return -1;
    }
	else if (pid == 0) //子プロセスには0が返る
	{
		//ToDo:renameat関数使ってリネームできそう
		printf("TcpServer_path = [%s]\n", TcpServer_path);
		nRet = execv(TcpServer_path, str);
        if ( nRet == -1 ) 
		{
			printf("execv has failed in Start.cpp. errno=%d\n", errno);
			return -1;
        }
    }

	//TcpServerプロセスから正常起動完了報告を名前付きパイプで受ける
    /* 読取専用でパイプを開く */
    if ((fd_start = open(PIPE_START, O_RDONLY)) == -1)
    {
		closeStartPipe();
        return 1;
    }

    //TcpServer立ち上がり完了報告を読取
    bool flag = true;

    while (flag == true){
        if((nbyte = read(fd_start, buff, sizeof(buff))) > 0){
            write(fileno(stdout), buff, nbyte);
			printf("\n");
            flag = false;
        }else if(nbyte == -1){
			perror("write()\n");
			closeStartPipe();
			return -1;
		}
    }

	closeStartPipe();
	return 0;
}

int closeStartPipe(){
	if(close(fd_start) != 0) perror("close()\n");
    unlink(PIPE_START);
	return(0);
};
#endif