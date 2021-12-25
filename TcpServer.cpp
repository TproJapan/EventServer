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
//#define BOOST_LOG_DYN_LINK 1 //konishi makefileのCCFLAGSに引っ越し
#include "BoostLog.h"
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "thread_pool.h"
#define MAXFD 64
///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
bool main_thread_flag = true;

//std::thread vector
//typedef std::vector<std::thread::id> threadid_vector;
//threadid_vector threadid_vec;//thread id管理Vector配列

//pthread_t vector
//typedef std::vector<pthread_t> pthreadid_vector;
//pthreadid_vector pthreadid_vec;//pthread id管理Vector配列

int fd_start;//Startへの立ち上がり完了報告用名前付きパイプ;
int fddevnull = 0;//dev/null用fd

///////////////////////////////////////////////////////////////////////////////
// 関数
///////////////////////////////////////////////////////////////////////////////
void sigalrm_handler(int signo, thread_pool _tp);
void sigusr2_handler(int signo);

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
//memory解放処理するためにConnectClient*のList変数で管理
typedef std::vector<ConnectClient*> connectclient_vector;
connectclient_vector connectclient_vec;

int main(int argc, char* argv[])
{
	//BoostLog有効化
	init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
	logging::add_common_attributes();

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
	//char log_path[255];
	//sprintf(log_path, "%s/Log", PROJ_HOME);


    // カレントディレクトリ変更.
    // ルートディレクトリに移動.(デーモンプロセスはルートディレクトリを起点にして作業するから)
    chdir("/");

    // 親から引き継いだ全てのファイルディスクリプタのクローズ.
    for(int i = 0; i < MAXFD; i++){
        close(i);
    }

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
	SetServerStatus(0);

	//Startプロセス通信用の名前つきパイプを書込専用で開き
    if ((fd_start = open(PIPE_START, O_WRONLY)) == -1)
    {
		write_log(4, "open PIPE_START failed\n");
        return -1;
    }

	//Startへプロセスid送信
	char buff[16];
	if (write(fd_start, buff, strlen(buff)) != strlen(buff))
	{
		write_log(4, "write PIPE_START failed\n");
		close(fd_start);
		return -1;
	}

	//スレッドプール作成
	boost::asio::io_service io_service;
	thread_pool tp(io_service, CLIENT_MAX);

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
	nRet = sigemptyset(&sigset);
    if( nRet != 0 ) return -1;

    //Control-C(SIGINT)で割り込まれないようにする
    nRet = sigaddset(&sigset, SIGINT);
    if( nRet != 0 ) return -1;
    act.sa_mask = sigset;

	///////////////////////////////////
    // SIGALRM捕捉
    ///////////////////////////////////
    //第1引数はシステムコール番号
    //第2引数は第1引数で指定したシステムコールで呼び出したいアクション
    //第3引数は第1引数で指定したシステムコールでこれまで呼び出されていたアクションが格納される。NULLだとこれまでの動作が破棄される
    nRet = sigaction(SIGALRM,&act,NULL);
    if ( nRet == -1 ) err(EXIT_FAILURE, "sigaction(sigalrm) error");


	///////////////////////////////////
    // SIGUSR2捕捉
    ///////////////////////////////////
	memset(&act, 0, sizeof(act));//再度初期化
	act.sa_handler = sigusr2_handler;
	nRet = sigaction(SIGUSR2,&act,NULL);
	if ( nRet == -1 ) err(EXIT_FAILURE, "sigaction(siguer2) error");

	///////////////////////////////////
    // socketの設定
    ///////////////////////////////////
    // listen用sockaddrの設定
	struct sockaddr_in srcAddr;
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(nPortNo);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	// ソケットの生成(listen用)
	int srcSocket;
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if ( srcSocket == -1 ) {
		//printf("socket error\n");
		write_log(4, "socket error\n");
		return -1;
	}

	// ソケットのバインド
	const int on = 1;

	//setsockoptは-1だと失敗
	nRet = setsockopt(srcSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if ( nRet == -1 ) {
		//printf("setsockopt error\n");
		write_log(4, "setsockopt error\n");
		return -1;
	}

	nRet = bind(srcSocket,
				(struct sockaddr *)&srcAddr,
				sizeof(srcAddr));
	if ( nRet == -1 ) {
		//printf("bind error\n");
		write_log(4, "bind error\n");
		return -1;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if ( nRet == -1 ) {
		//printf("listen error\n");
		write_log(4, "listen error\n");
		return -1;
	}

	while(main_thread_flag) {
	 	int dstSocket = -1;		// クライアントとの通信ソケット

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
				// perror("select");
				write_log(4, "select error\n");
				exit( 1 );
			}
	  	}else if( nRet == 0 ){
			//printf("selectでタイムアウト発生\n");
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
  			//printf("クライアント接続要求を受け付けました\n");
			write_log(2, "クライアント接続要求を受け付けました\n");

			struct sockaddr_in dstAddr;
			int dstAddrSize = sizeof(dstAddr);
			
			//新規クライアント用socket確保
			dstSocket = accept(srcSocket,
								(struct sockaddr *)&dstAddr, 
								(socklen_t *)&dstAddrSize);
			if ( dstSocket == -1 ) {
			  	// perror("accept");
				  write_log(4, "accept error\n");
			  	continue;	
			}
			
			// printf("[%s]から接続を受けました. socket=%d\n",
			// 		inet_ntoa(dstAddr.sin_addr),
			// 		dstSocket);
			write_log(2, "[%s]から接続を受けました. socket=%d\n",
					inet_ntoa(dstAddr.sin_addr),
					dstSocket);

        	// クライアントとの通信
			//ToDo:threadをnewしてvectorにpush_backしてみる
			//std::thread th{ConnectClient( dstSocket)};
			//const std::thread::id new_thread_id = th.get_id();
			
			//Vectorに保存しておく
			//threadid_vec.push_back(new_thread_id);
			//pthreadid_vec.push_back(th.native_handle());//C++11式、プラットフォーム固有のスレッドハンドラ取得

			//デタッチ
			//th.detach();

			//プールスレッドにバインド
			ConnectClient* h = new ConnectClient(dstSocket);
  			//vectorに追加
			connectclient_vec.push_back(h);

  			//printf("*** h=%p, dstSocket=%d\n",h, dstSocket);//konishi
			write_log(2, "*** h=%p, dstSocket=%d\n",h, dstSocket);
			tp.post(boost::bind(&ConnectClient::func, h));
  		}

		//不要なConnectClientを解放
		//for(const auto& item: connectclient_vec) {
		for(auto it = connectclient_vec.begin(); it != connectclient_vec.end();it++) {
			std::lock_guard<std::mutex> lk((*it)->m_mutex);
			write_log(2, "*** h=%p, flag=%d\n", *it, (*it)->_live);

			if((*it)->_live == false) {
				connectclient_vec.erase(it);//要素削除
				delete *it;//memory解放
			}
		}
	}

	// 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if ( nRet == -1 ) {
		//printf("close error\n");
		write_log(4, "close error\n");
	}
	
	//workerスレッドをクローズ
	// std::lock_guard<std::mutex> lk(server_status_Mutex);
	// server_status = 1;
	SetServerStatus(1);

	//sigalerm発行
	//ToDo:windowdだとイベントを起こす関数があるはずなのでそれを使い、それをコールバック処理でthreadを殺しにいく
	alarm(60);
	//printf("alarmがセットされました\n");
	write_log(2, "alarmがセットされました\n");

	int past_seconds = 0;

	while(1){
		sleep(2);
		past_seconds += 2;
		//printf("%d秒経過しました\n", past_seconds);
		write_log(2, "%d秒経過しました\n", past_seconds);

		bool result = connectclient_vec.empty();
		if(result == true){
			//printf("ループを抜けます\n");
			write_log(2, "ループを抜けます\n");
			break;
		}
	}

	//printf("正常終了します\n");
	write_log(2, "正常終了します\n");
	return(0);
}

void sigalrm_handler(int signo, thread_pool _tp)
{
	// char work[256];
	// sprintf(work, "sig_handler started. signo=%d\n", signo);
	write_log(2, "sig_handler started. signo=%d\n", signo);
	//pthreadid_vecに入っているスレッドidを直指定してスレッドを殺しにいく
	// C++11 Range based for
//	for(const auto& item: pthreadid_vec) {
//		std::cout << "pthread id:" << item << "\n";
//	}
//	
//	for(const auto& item: pthreadid_vec) {
//		int r = pthread_cancel(item);//pthread_killではなく
//		printf("pthread_cancel result = %d\n", r);
//		std::cout << "Killed pthread id:" << item << "\n";
//	}

	//ToDo:windowsだとpthreadではなくTerminateThread
	//Memo:signal,sigactionもlinuxのみの機構。

	//ワーカースレッド強制終了します
	_tp.terminateAllThreads();
	write_log(2, "ワーカースレッド強制終了しました\n");
	return;
}

void sigusr2_handler(int signo){
	// char work[256];
	// sprintf(work, "sig_handler started. signo=%d\n", signo);
	write_log(2, "sig_handler started. signo=%d\n", signo);

	main_thread_flag = false;
	//printf("main_thread_flagを書き換えました\n");
	write_log(2, "main_thread_flagを書き換えました\n");
	return;
}