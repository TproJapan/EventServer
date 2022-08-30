#ifndef _WIN64
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <iostream>
#include <sys/wait.h>// wait
#include <err.h>	// err
#include <stdlib.h>	// exit
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <pthread.h>
#include "BoostLog.h"
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "ThreadPool.h"
#include "TcpServer.h"
#define MAXFD 64

int fd_start;//Startへの立ち上がり完了報告用名前付きパイプ;
int fddevnull = 0;//dev/null用fd
TcpServer* tcpServer;

int main(int argc, char* argv[])
{
	//BoostLog有効化
	init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
	logging::add_common_attributes();
	writeLog(4, "main started , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	
	char work[256];
    pid_t s_pid;
	int nPortNo;

    //Startからポート番号を受け取る
    nPortNo = atoi(argv[1]);

    //Startからプロセスidを受け取る
	char* tmp = argv[2];

	//------------------------------------------------------
    // 状態.
    // 子プロセスのみとなったがTTYは保持したまま.
    //------------------------------------------------------


    //------------------------------------------------------
    // TTYを切り離してセッションリーダー化、プロセスグループリーダー化する.
    //------------------------------------------------------
    setsid();


    //------------------------------------------------------
    // HUPシグナルを無視.
    // 親が死んだ時にHUPシグナルが子にも送られる可能性があるため.
    //------------------------------------------------------
    signal(SIGHUP, SIG_IGN);
 	signal(SIGCHLD, SIG_IGN);


    //------------------------------------------------------
    // 状態.
    // このままだとセッションリーダーなのでTTYをオープンするとそのTTYが関連づけられてしまう.
    //------------------------------------------------------


    //------------------------------------------------------
    // 親プロセス(セッショングループリーダー)を切り離す.
    // 親を持たず、TTYも持たず、セッションリーダーでもない状態になる.
    //------------------------------------------------------
	pid_t pid = 0;
	pid = fork();
    if (pid == -1 ) 
	{
		writeLog(4, "Fork has failed, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return -1;
    }

    if(pid != 0){
        // 子供世代プロセスを終了し、孫世代になる.
        _exit(0);
    }

    //------------------------------------------------------
    // デーモンとして動くための準備を行う.
    //------------------------------------------------------
	//Logファイルパス取得
	char dir[255];
	getcwd(dir,255);


    // カレントディレクトリ変更.
    // ルートディレクトリに移動.(デーモンプロセスはルートディレクトリを起点にして作業するから)
    chdir("/");
	writeLog(4, "after cddir , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	
    // 親から引き継いだ全てのファイルディスクリプタのクローズ.
    for(int i = 0; i < MAXFD; i++){
        close(i);
    }

	writeLog(4, "before open /dev/null , %s %d %s\n", __FILENAME__, __LINE__, __func__);
    // stdin,stdout,stderrをdev/nullでオープン.
    // 単にディスクリプタを閉じるだけだとこれらの出力がエラーになるのでdev/nullにリダイレクトする.
    if((fddevnull = open("/dev/null", O_RDWR, 0) != -1)){

        // ファイルディスクリプタの複製.
        // このプロセスの0,1,2をfdが指すdev/nullを指すようにする.
        dup2(fddevnull, 0);
        dup2(fddevnull, 1);
        dup2(fddevnull, 2);
        if(fddevnull < 2){
            close(fddevnull);
        }
    }

    //------------------------------------------------------
    // デーモン化後の処理
    //------------------------------------------------------
	//初期化
	writeLog(4, "before setServerStatus , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	setServerStatus(0);
	writeLog(4, "after setServerStatus , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	
	//Startプロセス通信用の名前つきパイプを書込専用で開き
    if ((fd_start = open(PIPE_START, O_WRONLY)) == -1)
    {
		writeLog(4, "open PIPE_START failed, %s %d %s\n", __FILENAME__, __LINE__, __func__);
        return -1;
    }

	//Startへプロセスid送信
	char buff[16];
	sprintf(buff, "pid = %d", getpid());
	if (write(fd_start, buff, strlen(buff)) != strlen(buff))
	{
		writeLog(4, "write PIPE_START failed, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		close(fd_start);
		return -1;
	}
	writeLog(4, "after pipe wirite , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	
	//TcpServer作成
	try{
		tcpServer = new TcpServer(nPortNo);
	}
	catch(int e){
		return -1;
	}

	tcpServer->Func();

	writeLog(2, "Before Destructor run , %s %d %s\n", __FILENAME__, __LINE__, __func__);
	delete tcpServer;
	writeLog(2, "After Destructor run , %s %d %s\n", __FILENAME__, __LINE__, __func__);

	return(0);
}
#endif