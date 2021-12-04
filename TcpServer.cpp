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
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "thread_pool.h"
///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
bool main_thread_flag = true;

//std::thread vector
typedef std::vector<std::thread::id> threadid_vector;
threadid_vector threadid_vec;//thread id管理Vector配列

//pthread_t vector
typedef std::vector<pthread_t> pthreadid_vector;
pthreadid_vector pthreadid_vec;//pthread id管理Vector配列

#define CLIENT_MAX	32 //マシーンリソースに依存する数
#define SELECT_TIMER_SEC	3			// selectのタイマー(秒)
#define SELECT_TIMER_USEC	0			// selectのタイマー(マイクロ秒)
///////////////////////////////////////////////////////////////////////////////
// 関数
///////////////////////////////////////////////////////////////////////////////
void sigalrm_handler(int signo);
void sigusr2_handler(int signo);

std::mutex mtx;
int GetServerStatus(){
	std::lock_guard<std::mutex> lock(mtx);
	return server_status;
}
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	int  _dstSocket;
	bool _live;//生存管理フラグ
	public:
		ConnectClient(int dstSocket){
			_dstSocket = dstSocket;
			_live = true;
		};
		~ConnectClient(){
			std::thread::id this_id = std::this_thread::get_id();
			auto it = std::find(threadid_vec.begin(), threadid_vec.end(), this_id);

			if(it != threadid_vec.end()) {
				//C++11らしい記述で、pthread_tをvectorから削除
				size_t index = std::distance(threadid_vec.begin(), it);
				pthreadid_vec.erase(pthreadid_vec.begin() + index);

				//std::thread::idをvectorから削除
				threadid_vec.erase(it);
			}
		};
	public: 
		void operator()(){
			while(1){
				///////////////////////////////////
				// 排他制御でserver_statusチェック
				///////////////////////////////////

				//if(GetServerStatus() != 0) _live = false;

				//終了確認
				if(_live == false){
					std::cout << "Thread:" << std::this_thread::get_id() << "を終了します" << std::endl;
					return;
				}

				///////////////////////////////////
				// 通信
				///////////////////////////////////
				printf("client(%d)クライアントとの通信を開始します\n", _dstSocket);
				size_t stSize;
				char buf[1024];
				
				// タイムアウトの設定
				struct timeval  tval;
				tval.tv_sec  = SELECT_TIMER_SEC	;	// time_t  秒
				tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒

				fd_set  readfds;//ビットフラグ管理変数
				FD_ZERO(&readfds);//初期化

				FD_SET(_dstSocket, &readfds);

				int nRet = select(FD_SETSIZE,
								&readfds,
								NULL,
								NULL,
								&tval );

				if( nRet == -1 ) {
					if(errno == EINTR){//シグナル割り込みは除外
						continue;
					}else{
						// selectが異常終了
						//システムコールのエラーを文字列で標準出力してくれる
						//エラーメッセージの先頭に"select"と表示される(自分用の目印))
						perror("select");
						exit( 1 );
					}
				}else if ( nRet == 0 ) {
					printf("workerスレッドでタイムアウト発生\n");
					continue;
				}

				stSize = recv(_dstSocket,
							buf,
							sizeof(buf),
							0);
				if ( stSize <= 0 ) {
					printf("recv error.\n");
					printf("クライアント(%d)との接続が切れました\n", _dstSocket);
					close(_dstSocket);
					return;
				}
				
				printf("変換前:[%s] ==> ", buf);
				for (int i=0; i< stSize; i++){ // bufの中の小文字を大文字に変換
					if ( isalpha(buf[i])) {
					buf[i] = toupper(buf[i]);
					}
				}
				
				// クライアントに返信
				stSize = send(_dstSocket,
							buf,
							strlen(buf)+1,
							0);
				
				if ( stSize != strlen(buf)+1) {
					printf("send error.\n");
					printf("クライアントとの接続が切れました\n");
					close(_dstSocket);
					return;
				}
				printf( "変換後:[%s] \n" ,buf);
			}
		}
};
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	init(0, LOG_DIR_SERV, LOG_FILENAME_SERV);
	//logging::add_common_attributes();

	///////////////////////////////////
    // コマンド引数の解析
    ///////////////////////////////////
    if ( argc != 2 ) {
      printf("TcpServer portNo\n");
      return -1;
    }
	write_log(2, "ログ書き込み成功\n");

	//スレッドプール作成
	boost::asio::io_service io_service;
	thread_pool tp(io_service, CLIENT_MAX);
	
    // ポート番号の設定
    int nPortNo;            // ポート番号
    nPortNo = atol(argv[1]);

	///////////////////////////////////
    // シグナルハンドラの設定
    ///////////////////////////////////
    struct sigaction act;
    memset(&act, 0, sizeof(act)); //メモリにゴミが入っているので初期化
    act.sa_handler = sigalrm_handler;
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
		printf("socket error\n");
		return -1;
	}

	// ソケットのバインド
	const int on = 1;

	//setsockoptは-1だと失敗
	nRet = setsockopt(srcSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if ( nRet == -1 ) {
		printf("setsockopt error\n");
		return -1;
	}

	nRet = bind(srcSocket,
				(struct sockaddr *)&srcAddr,
				sizeof(srcAddr));
	if ( nRet == -1 ) {
		printf("bind error\n");
		return -1;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if ( nRet == -1 ) {
		printf("listen error\n");
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
				
		printf("新規接続とクライアントから書き込みを待っています.\n");	


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
				perror("select");
				exit( 1 );
			}
	  	}else if( nRet == 0 ){
			printf("selectでタイムアウト発生\n");
			continue;
  		}

		///////////////////////////////////////
  		// 反応のあったソケットをチェック
		///////////////////////////////////////
  		 // 新規のクライアントから接続要求がきた
  		if ( FD_ISSET(srcSocket, &readfds) ) {
  			printf("クライアント接続要求を受け付けました\n");

			struct sockaddr_in dstAddr;
			int dstAddrSize = sizeof(dstAddr);
			
			//新規クライアント用socket確保
			dstSocket = accept(srcSocket,
								(struct sockaddr *)&dstAddr, 
								(socklen_t *)&dstAddrSize);
			if ( dstSocket == -1 ) {
			  	perror("accept");
			  	continue;	
			}
			
			printf("[%s]から接続を受けました. socket=%d\n",
					inet_ntoa(dstAddr.sin_addr),
					dstSocket);

        	// クライアントとの通信
			//ToDo:threadをnewしてvectorにpush_backしてみる
			std::thread th{ConnectClient( dstSocket)};
			const std::thread::id new_thread_id = th.get_id();
			
			//Vectorに保存しておく
			threadid_vec.push_back(new_thread_id);
			pthreadid_vec.push_back(th.native_handle());//C++11式、プラットフォーム固有のスレッドハンドラ取得

			//デタッチ
			th.detach();
  		}
	}

	// 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if ( nRet == -1 ) {
		printf("close error\n");
	}
	
	//workerスレッドをクローズ
	server_status = 1;

	//sigalerm発行
	//ToDo:windowdだとイベントを起こす関数があるはずなのでそれを使い、それをコールバック処理でthreadを殺しにいく
	alarm(60);
	printf("alarmがセットされました\n");

	int past_seconds = 0;

	while(1){
		sleep(2);
		past_seconds += 2;
		printf("%d秒経過しました\n", past_seconds);

		bool result = threadid_vec.empty();
		if(result == true){
			printf("ループを抜けます\n");
			break;
		}
	}

	printf("正常終了します\n");
	return(0);
}

void sigalrm_handler(int signo)
{
	char work[256];
	sprintf(work, "sig_handler started. signo=%d\n", signo);

	//pthreadid_vecに入っているスレッドidを直指定してスレッドを殺しにいく
	// C++11 Range based for
	for(const auto& item: pthreadid_vec) {
		std::cout << "pthread id:" << item << "\n";
	}
	
	for(const auto& item: pthreadid_vec) {
		int r = pthread_cancel(item);//pthread_killではなく
		printf("pthread_cancel result = %d\n", r);
		std::cout << "Killed pthread id:" << item << "\n";
	}

	//ToDo:windowsだとpthreadではなくTerminateThread
	//Memo:signal,sigactionもlinuxのみの機構。

	return;
}

void sigusr2_handler(int signo){
	char work[256];
	sprintf(work, "sig_handler started. signo=%d\n", signo);

	main_thread_flag = false;
	printf("main_thread_flagを書き換えました\n");
	return;
}