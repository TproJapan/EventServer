#pragma once
#include "BoostLog.h"
#include "TcpCommon.h"

///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	private:
		int  _dstSocket;
	public:
		bool _live;//生存管理フラグ
		std::mutex	m_mutex;//生存管理フラグ用排他
	public:
		ConnectClient(int dstSocket){
			_dstSocket = dstSocket;
			_live = true;
		};
		~ConnectClient(){
		};
	public: 
		void func(){
			//printf("func started. _dstSocket=%d\n", _dstSocket);// konishi
			write_log(2, "func started. _dstSocket=%d\n", _dstSocket);

			while(1){
				///////////////////////////////////
				// 排他制御でserver_statusチェック
				///////////////////////////////////

				//if(GetServerStatus() != 0) _live = false;

				//終了確認
				if(GetServerStatus() == 1){
					//std::cout << "Thread:" << std::this_thread::get_id() << "を終了します" << std::endl;
					write_log(2, "Thread:%sを終了します\n", std::this_thread::get_id());
					break;
				}

				///////////////////////////////////
				// 通信
				///////////////////////////////////
				//printf("client(%d)クライアントとの通信を開始します\n", _dstSocket);
				write_log(2, "client(%d)クライアントとの通信を開始します\n", _dstSocket);
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
						//perror("select");
						write_log(4, "select error\n");
						break;
					}
				}else if ( nRet == 0 ) {
					//printf("workerスレッドでタイムアウト発生\n");
					write_log(2, "workerスレッドでタイムアウト発生\n");
					continue;
				}

				stSize = recv(_dstSocket,
							buf,
							sizeof(buf),
							0);
				if ( stSize <= 0 ) {
					//printf("recv error.\n");
					write_log(4, "recv error.\n");
					//printf("クライアント(%d)との接続が切れました\n", _dstSocket);
					write_log(4, "クライアント(%d)との接続が切れました\n", _dstSocket);
					close(_dstSocket);
					break;
				}
				
				//printf("変換前:[%s] ==> ", buf);
				write_log(2, "変換前:[%s] ==> ", buf);
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
					//printf("send error.\n");
					write_log(4, "send error.\n");
					//printf("クライアントとの接続が切れました\n");
					write_log(4, "クライアントとの接続が切れました\n");
					close(_dstSocket);
					break;
				}
				//printf( "変換後:[%s] \n" ,buf);
				write_log(2, "変換後:[%s] \n" ,buf);
			}
			//while抜けたらフラグ倒す
			std::lock_guard<std::mutex> lk(m_mutex);
			_live = false;
		}
};