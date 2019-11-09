///////////////////////////////////////////////////////////////////////////////
//  TcpServer
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/select.h>

#include <sys/wait.h>// wait
#include <err.h>	// err
#include <stdlib.h>	// exit
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <mutex>

//gloabl変数
std::mutex clientSockMutex_;

///////////////////////////////////////////////////////////////////////////////
// define
///////////////////////////////////////////////////////////////////////////////
#define CLIENT_MAX	32 //マシーンリソースに依存する数
#define SELECT_TIMER_SEC	3			// selectのタイマー(秒)
#define SELECT_TIMER_USEC	0			// selectのタイマー(マイクロ秒)
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	int _nSockNo, _dstSocket;
	int* _clientSock;
	public:
		ConnectClient(int nSockNo, int dstSocket, int* clientSock){
			_nSockNo = nSockNo;
			_dstSocket = dstSocket;
			_clientSock = clientSock;
		};
		~ConnectClient(){};
	public: 
		void operator()(){
			//clientSockは書き換える可能性あるからmutexガード
			std::lock_guard<std::mutex> lk(clientSockMutex_);

        	printf("client(%d)クライアントとの通信を開始します\n", _dstSocket);
        	size_t stSize;
        	char buf[1024];
			
        	// クライアントから受信
        	stSize = recv(_dstSocket,
            	          buf,
                	      sizeof(buf),
                    	  0);
        	if ( stSize <= 0 ) {
          		printf("recv error.\n");
          		printf("クライアント(%d)との接続が切れました\n", _dstSocket);
          		close(_dstSocket);
          		_clientSock[_nSockNo] = -1;
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
          		_clientSock[_nSockNo] = -1;
          		return;
        	}
			printf( "変換後:[%s] \n" ,buf);
		}
};
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	///////////////////////////////////
    // コマンド引数の解析
    ///////////////////////////////////
    if ( argc != 2 ) {
      printf("TcpServer portNo\n");
      return -1;
    }

    // ポート番号の設定
    int nPortNo;            // ポート番号
    nPortNo = atol(argv[1]);
    
    // 接続中のソケット管理配列の初期化
	int clientSock[CLIENT_MAX];
	memset(&clientSock, -1, sizeof(clientSock));

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
	int nRet;
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

	while(1) {
	 	int dstSocket = -1;		// クライアントとの通信ソケット

		///////////////////////////////////////
		// selectで監視するソケットの登録
		///////////////////////////////////////
		fd_set  readfds;
		FD_ZERO(&readfds);//初期化

		// listen用ソケット(接続待ち)
		FD_SET(srcSocket, &readfds);

		// 接続済みソケットの登録
		for(int i=0; i< CLIENT_MAX; i++) {
			if ( clientSock[i] != -1 ) {//-1は未接続なソケット
				FD_SET( clientSock[i], &readfds);//有効なソケット用にビットを立てる
			}
		}

		// タイムアウトの設定
		struct timeval  tval;
		tval.tv_sec  = SELECT_TIMER_SEC	;	// time_t  秒
    	tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒
				
		printf("新規接続とクライアントから書き込みを待っています.\n");	


		//msgrsv:引数でmsgflg に IPC_NOWAIT

		//第一引数は一番大きなディスクリプター番号
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
			// selectが異常終了
			//システムコールのエラーを文字列で標準出力してくれる
			//エラーメッセージの先頭に"select"と表示される(自分用の目印))
  			perror("select");
  			exit( 1 );
  		}
  		else if ( nRet == 0 ) {
  			printf("selectでタイムアウト発生\n");
  			continue;
  		}

		///////////////////////////////////////
  		// 反応のあったソケットをチェック
		///////////////////////////////////////
  		 // 新規のクライアントから接続要求がきたかどうか確認
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

			// クライアントとの通信ソケットを管理配列に保存
			for(int i=0; i< CLIENT_MAX; i++) {
				if ( clientSock[i] == -1 ) {
					clientSock[i] = dstSocket;
					break;
				}
			}
  		}

  		// 既存クライアントからの書き込み要求
		for(int nSockNo=0; nSockNo < CLIENT_MAX; nSockNo++) {
			if ( clientSock[nSockNo] == -1 ) {
				// 無効なソケットなのでスキップ
				continue;
			}

			dstSocket = clientSock[nSockNo];
			if ( !FD_ISSET(dstSocket, &readfds) ) {
				// 入力がないのでスキップ
				continue;
			}

        	// クライアントとの通信
			std::thread th{ConnectClient(nSockNo, dstSocket, clientSock)};
			th.join();
		} 	
	}

	// クライアントとの通信ソケットをクローズ
	for(int nSockNo=0; nSockNo < CLIENT_MAX; nSockNo++) {
		if ( clientSock[nSockNo] == -1 ) {
			// 無効なソケットなのでスキップ
			continue;
		}
		close(clientSock[nSockNo]);
	}

	// 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if ( nRet == -1 ) {
		printf("close error\n");
		return -1;
	}
	
	return(0);
}
