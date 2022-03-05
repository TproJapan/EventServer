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
#include "thread_pool.h"
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
	write_log(4, "konishi : main started \n");
	
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
		write_log(4, "Fork has failed in EventServer.cpp\n");
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
	write_log(4, "konishi : after cddir \n");// konishi
	
    // 親から引き継いだ全てのファイルディスクリプタのクローズ.
    for(int i = 0; i < MAXFD; i++){
        close(i);
    }

	write_log(4, "konishi : before open /dev/null \n");// konishi
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
	write_log(4, "konishi : before SetServerStatus \n");// konishi
	SetServerStatus(0);
	write_log(4, "konishi : after SetServerStatus \n");// konishi
	
	//Startプロセス通信用の名前つきパイプを書込専用で開き
    if ((fd_start = open(PIPE_START, O_WRONLY)) == -1)
    {
		write_log(4, "open PIPE_START failed\n");
        return -1;
    }

	//Startへプロセスid送信
	char buff[16];
	sprintf(buff, "pid = %d", getpid());// konishi
	if (write(fd_start, buff, strlen(buff)) != strlen(buff))
	{
		write_log(4, "write PIPE_START failed\n");
		close(fd_start);
		return -1;
	}
	write_log(4, "konishi : after pipe wirite \n");// konishi
	
	//TcpServer作成
	try{
		tcpServer = new TcpServer(nPortNo);
	}
	catch(int e){
		return -1;
	}

	tcpServer->Func();

	return(0);
}