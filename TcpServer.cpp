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

bool main_thread_flag = true;
//memory解放処理するためにConnectClient*のList変数で管理
connectclient_vector connectclient_vec;

void sigalrm_handler(int signo, thread_pool _tp)
{
	write_log(2, "sig_handler started. signo=%d\n", signo);
	//ワーカースレッド強制終了します
	_tp.terminateAllThreads();
	write_log(2, "ワーカースレッド強制終了しました\n");
	return;
}

void sigusr2_handler(int signo){
	write_log(2, "sig_handler started. signo=%d\n", signo);
	main_thread_flag = false;
	write_log(2, "main_thread_flagを書き換えました\n");
	return;
}

TcpServer::TcpServer(int portNo):tp(io_service, CLIENT_MAX) {
    nPortNo = portNo;

    ///////////////////////////////////
    // シグナルハンドラの設定
    ///////////////////////////////////
    struct sigaction act;
    memset(&act, 0, sizeof(act)); //メモリにゴミが入っているので初期化
    act.sa_handler = (__sighandler_t)sigalrm_handler;
    act.sa_flags = SA_RESTART; //何度シグナルが来てもハンドラ実行を許可する

	///////////////////////////////////
    // 割り込みを抑止するシグナルの設定
    ///////////////////////////////////
    sigset_t sigset; // シグナルマスク
    int nRet = 0;

    //シグナルマスクの初期化
	write_log(4, "konishi : before sigemptyset \n");// konishi
	nRet = sigemptyset(&sigset);
    if( nRet != 0 ) throw -1;

    //Control-C(SIGINT)で割り込まれないようにする
	write_log(4, "konishi : before sigaddset \n");// konishi
    nRet = sigaddset(&sigset, SIGINT);
    if( nRet != 0 ) throw -1;
    act.sa_mask = sigset;

	write_log(4, "konishi : Before sigaction \n");// konishi
	///////////////////////////////////
    // SIGALRM捕捉
    ///////////////////////////////////
    //第1引数はシステムコール番号
    //第2引数は第1引数で指定したシステムコールで呼び出したいアクション
    //第3引数は第1引数で指定したシステムコールでこれまで呼び出されていたアクションが格納される。NULLだとこれまでの動作が破棄される
    nRet = sigaction(SIGALRM,&act,NULL);
    if ( nRet == -1 ) {
        write_log(4, "sigaction(sigalrm) error");
        throw -1;
    }

	///////////////////////////////////
    // SIGUSR2捕捉
    ///////////////////////////////////
	memset(&act, 0, sizeof(act));//再度初期化
	act.sa_handler = sigusr2_handler;
	nRet = sigaction(SIGUSR2,&act,NULL);
	if ( nRet == -1 ) {
        write_log(4, "sigaction(sigalrm2) error");
        throw -1;
    }

	///////////////////////////////////
    // socketの設定
    ///////////////////////////////////
    // listen用sockaddrの設定
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(nPortNo);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	// ソケットの生成(listen用)
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if ( srcSocket == -1 ) {
		write_log(4, "socket error\n");
		throw -1;
	}

	// ソケットのバインド
	const int on = 1;

	//setsockoptは-1だと失敗
	nRet = setsockopt(srcSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if ( nRet == -1 ) {
		write_log(4, "setsockopt error\n");
		throw -1;
	}

	nRet = bind(srcSocket,
				(struct sockaddr *)&srcAddr,
				sizeof(srcAddr));
	if ( nRet == -1 ) {
		write_log(4, "bind error\n");
		throw -1;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if ( nRet == -1 ) {
		write_log(4, "listen error\n");
		throw -1;
	}
}

TcpServer::~TcpServer() {
    int nRet = 0;
    
    // 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if ( nRet == -1 ) {
		write_log(4, "close error\n");
	}
	
	SetServerStatus(1);

	//sigalerm発行
	alarm(60);
	write_log(2, "alarmがセットされました\n");

	int past_seconds = 0;

	while(1){
		sleep(2);
		past_seconds += 2;
		write_log(2, "%d秒経過しました\n", past_seconds);

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);
		bool result = connectclient_vec.empty();
		if(result == true){
			write_log(2, "ループを抜けます\n");
			break;
		}
	}

	write_log(2, "正常終了します\n");
}

int TcpServer::Func() {	
    int nRet = 0;
	
	while(main_thread_flag) {
	 	int dstSocket = -1;		// クライアントとの通信ソケット

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec); // konishi
		
		///////////////////////////////////////
		// selectで監視するソケットの登録
		///////////////////////////////////////
		fd_set  readfds;//ビットフラグ管理変数
		FD_ZERO(&readfds);//初期化

		// readfdsにlisten用ソケットを登録。後でFD_ISSETでビットが立っていれば新規接続があったという事
		FD_SET(srcSocket, &readfds);

		// タイムアウトの設定
		struct timeval  tval;
		tval.tv_sec  = SELECT_TIMER_SEC	;	// time_t  秒
    	tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒
				
		//printf("新規接続とクライアントから書き込みを待っています.\n");	
		write_log(2, "新規接続とクライアントから書き込みを待っています.\n");

		//msgrsv:引数でmsgflg に IPC_NOWAIT

		//第一引数はシステムがサポートするデスクリプタの最大数
		//タイムアウト不要の場合は第5引数にnull
		//第二引数は読み込み用FDS
		//第3引数は書き込み用fds
		//第四引数は実行可能か判定するfds
		//第五引数はタイマー(timeval構造体)のアドレス
		nRet = select(FD_SETSIZE,
						&readfds,
						NULL,
						NULL,
						&tval );

		if( nRet == -1 ) {
			if(errno == EINTR){//シグナル割り込みは除外
				continue;
			}else{
				// selectが異常終了
				write_log(4, "select error\n");
				exit( 1 );
			}
	  	}else if( nRet == 0 ){
			write_log(2, "selectでタイムアウト発生\n");
			continue;
  		}

		///////////////////////////////////////
  		// 反応のあったソケットをチェック
		///////////////////////////////////////

		//TODO 現在接続数がCLIANT_MAX数より少ないかチェック
		//多かったら新規接続を受け付けない

  		 // 新規のクライアントから接続要求がきた
  		if ( FD_ISSET(srcSocket, &readfds) ) {
			write_log(2, "クライアント接続要求を受け付けました\n");

			struct sockaddr_in dstAddr;
			int dstAddrSize = sizeof(dstAddr);
			
			//新規クライアント用socket確保
			dstSocket = accept(srcSocket,
								(struct sockaddr *)&dstAddr, 
								(socklen_t *)&dstAddrSize);
			if ( dstSocket == -1 ) {
				  write_log(4, "accept error\n");
			  	continue;	
			}
			
			write_log(2, "[%s]から接続を受けました. socket=%d\n",
					inet_ntoa(dstAddr.sin_addr),
					dstSocket);

			//プールスレッドにバインド
			ConnectClient* h = new ConnectClient(dstSocket);
  			//vectorに追加
			connectclient_vec.push_back(h); //konishi

			write_log(2, "*** h=%p, dstSocket=%d\n",h, dstSocket);
			tp.post(boost::bind(&ConnectClient::func, h));
  		}
    }

	return(0);
}

int TcpServer::cleanupConnectClientVec(connectclient_vector& vec)
{
	int deleteCount = 0;
	write_log(2, "konishi *** ゴミ掃除開始 ***");
	
	auto it = vec.begin();
	while( it != vec.end() ) {
		std::lock_guard<std::mutex> lk((*it)->m_mutex);
		write_log(2, "*** h=%p, flag=%d", *it, (*it)->_live);
		if((*it)->_live == false) {
			write_log(2, "konishi *** h=%p deleted", *it);
			delete *it;
			it = vec.erase(it);
			deleteCount++;
		}
		else{
			it++;
		}
	}
	
	write_log(2, "konishi *** deleteCount = %d ***", deleteCount);
	return deleteCount;
}