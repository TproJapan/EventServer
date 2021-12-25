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
#include "semlock.h"
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
	//排他制御
	int nRetS = 0;

	printf("Start is Waiting for resolving the Lock...\n");

	nRetS = sem_lock(LOCK_ID_TYPE01,
		IPC_CREAT);
	if (nRetS != 0) {
		printf("sem_lock error in Start.cpp. nRet=%d\n", nRetS);
		return -1;
	}
	printf("Locked by Start Process...\n");
	
	//pidファイルチェック
	nRet = exist_process(PID_SERVER);
	if(nRet == -1){
		printf("No pid file\n");
		printf("Start Command executed\n");
	}else if(nRet == 0){
		printf("pid file exist but process is no exist\n");
		printf("Start Command executed\n");
	}else{
		printf("process is already exist\n");

		//ToDo:ps aux | grep TcpServerを実行してTcpServerという文字があるか確認
		//system("ps aux | grep TcpServer | grep -v grep | grep -v Log > /tmp/hoge.txt");
		//hoge.txtをfile openして文字列処理を行う

		printf("Start Command stopped\n");
		semUnLock(LOCK_ID_TYPE01);
		return -1;
	}

	//引数チェック
	if ( argc == 1 || argc > 3 ) {
		perror("Start Option error\n");
		printf("E.G: # ./Start 5000 or # ./Start -f 5000\n");
		semUnLock(LOCK_ID_TYPE01);
		return -1;
	} 

	// 入力パイプの作成
	unlink(PIPE_START);
	nRet = mkfifo(PIPE_START, 0666);
	if ( nRet ==-1 ) {
		perror("mkfifo\n");
		semUnLock(LOCK_ID_TYPE01);
		return -1;
	}

	//int nPortNo = atol(argv[1]);
	
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
	sprintf(TcpServer_path, "%s%s",PROJ_HOME, "/TcpServer");
	char* const str[] = {(char*)"myServer", port_buff, pid_buff, NULL};
	pid_t pid = 0;
	
	pid = fork();

    if (pid == -1 ) 
	{
        printf("fork has failed in Start.cpp\n");
		semUnLock(LOCK_ID_TYPE01);
		return -1;
    }
	else if (pid == 0) //子プロセスには0が返る
	{
		//ToDo:renameat関数使ってリネームできそう
		printf("TcpServer_path = [%s]\n", TcpServer_path);//konishi
		nRet = execv(TcpServer_path, str);
        if ( nRet == -1 ) 
		{
			printf("execv has failed in Start.cpp. errno=%d\n", errno); //konishi
			semUnLock(LOCK_ID_TYPE01);
			return -1;
        }
    }

	//TcpServerプロセスから正常起動完了報告を名前付きパイプで受ける
    /* 読取専用でパイプを開く */
    if ((fd_start = open(PIPE_START, O_RDONLY)) == -1)
    {
		closeStartPipe();
		semUnLock(LOCK_ID_TYPE01);
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
			semUnLock(LOCK_ID_TYPE01);
			return -1;
		}
    }

	closeStartPipe();

	semUnLock(LOCK_ID_TYPE01);

	return 0;
}

int closeStartPipe(){
	if(close(fd_start) != 0) perror("close()\n");
    unlink(PIPE_START);
	return(0);
};