///////////////////////////////////////////////////////////////////////////////
// WinSockを使用したTCPサーバー
// Boost.Asioでスレッドプール
///////////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <boost/asio.hpp>
#include "thread_pool.h"
#include "ConnectClient.h"
#include "CommonFunc.h"
#define __MAIN_SRC__
#include "CommonVariables.h"

///////////////////////////////////////////////////////////////////////////////
// メイン処理
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	int nRet = 0;

	///////////////////////////////////
	// コマンド引数の解析
	///////////////////////////////////
	if (argc != 2) {
		printf("TcpServer portNo\n");
		return -1;
	}

	//スレッドプール作成
	boost::asio::io_service io_service;
	thread_pool tp(io_service, CLIENT_MAX);

	pSocketMap = CSocketMap::getInstance();

	// パイプ名の組み立て
	char pipeName[80];
	wsprintf(pipeName, PIPE_NAME, ".");

	// 名前付きパイプの作成
	HANDLE hPipe;
	hPipe = CreateNamedPipe(pipeName,		// パイプ名
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,		// パイプのアクセスモード
		PIPE_TYPE_MESSAGE,		// パイプの種類, 待機モード
		1,		// インスタンスの最大数
		0,		// 出力バッファのサイズ
		0,		// 入力バッファのサイズ
		150,	// タイムアウト値
		(LPSECURITY_ATTRIBUTES)NULL);	// セキュリティ属性
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	// OVARLAPPED構造体の初期設定
	OVERLAPPED overlappedConnect;
	memset(&overlappedConnect, 0, sizeof(overlappedConnect));
	HANDLE eventConnect = CreateEvent(0, FALSE, FALSE, 0);
	if (eventConnect == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}
	overlappedConnect.hEvent = eventConnect;

	// パイプクライアントからの接続を待つ。
	// ただしOVERLAP指定なのでクライアントからの接続が無くても正常終了する
	// (実際の接続確認はWSAWaitForMultipleEvents, GetOverlappedResultで行う)
	BOOL bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
	if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {	// konishi
		printf("ConnectNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	// ポート番号の設定
	int nPortNo;            // ポート番号
	nPortNo = atol(argv[1]);

	// WINSOCKの初期化(これやらないとWinSock2.hの内容が使えない)
	WSADATA	WsaData;
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup(wVersionRequested, &WsaData) != 0) {
		printf("WSAStartup() error. code=%d\n", WSAGetLastError());
		return -1;
	}

	// ソケットの生成(listen用)
	SOCKET srcSocket;
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srcSocket == -1) {
		printf("socket error\n");
		return -1;
	}

	HANDLE hEvent = WSACreateEvent();
	if (hEvent == INVALID_HANDLE_VALUE) {
		printf("WSACreateEvent error\n");
		return -1;
	}

	// UnixのSelectのような意味合いではない。該当ソケットはどのイベントにのみ反応するのかを定義する関数。
	// クライアントからの接続待ちソケットなのでFD_ACCEPTのみ関連付ける
	nRet = WSAEventSelect(srcSocket, hEvent, FD_ACCEPT);
	if (nRet == SOCKET_ERROR)
	{
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	///////////////////////////////////
	// socketの設定
	///////////////////////////////////
	// listen用sockaddrの設定
	struct sockaddr_in srcAddr;
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(nPortNo);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	// ソケットのバインド
	nRet = bind(srcSocket,(struct sockaddr*)&srcAddr,sizeof(srcAddr));
	if (nRet == SOCKET_ERROR) {
		printf("bind error. (%ld)\n", WSAGetLastError());
		return -1;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if (nRet == SOCKET_ERROR) {
		printf("listen error. (%ld)\n", WSAGetLastError());
		return -1;
	}

	pSocketMap->addSocket(srcSocket, hEvent);//listenソケットとイベントを結び付ける
	pSocketMap->addSocket(NULL,eventConnect);//名前付きパイプに対する接続待ちハンドルには対応ソケットは無いのでNULL

	while (1) {
		if (checkServerStatus() == 1) break;

		//イベントを配列で管理
		const int mainEventNum = 2;
		HANDLE eventList[mainEventNum];
		eventList[0] = hEvent;//listenイベントのみ
		eventList[1] = eventConnect;//名前付きパイプ

		printf("新規接続を待っています.\n");
		//DWORD dwTimeout = WSA_INFINITE;	// 無限待ち
		DWORD dwTimeout = TIMEOUT_MSEC;

		nRet = WSAWaitForMultipleEvents(mainEventNum, eventList, FALSE, dwTimeout, FALSE);
		if (nRet == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			break;
		}

		if (nRet == WSA_WAIT_TIMEOUT) {
			printf("タイムアウト発生!!!\n");
			continue;
		}

		printf("WSAWaitForMultipleEvents nRet=%ld\n", nRet);

		// イベントを検知したHANDLE
		HANDLE mainHandle = eventList[nRet];

		//ハンドル書き込み検出
		if (mainHandle == eventConnect) {
			printf("パイプに接続要求を受けました\n");

			DWORD byteTransfer;
			DWORD NumBytesRead;
			DWORD dwRet = 0;
			BOOL bRet;

			bRet = GetOverlappedResult(mainHandle, &overlappedConnect, &byteTransfer, TRUE);
			printf("GetOverlappedResult bRet = %ld\n", bRet);
			if (bRet != TRUE) {
				printf("GetOverlappedResult error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}
			ResetEvent(mainHandle);

			// パイプクライアントからのメッセージを受信
			char buf[1024];
			bRet = ReadFile(hPipe,
				buf,
				sizeof(buf),
				&NumBytesRead,
				(LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				printf("ReadFile error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}

			printf("クライアントと接続しました:[%s]\n", buf);

			// 受信メッセージが "stop" の場合はTcpServerを停止
			if (strcmp(buf, "stop") == 0) {
				std::lock_guard<std::mutex> lk(server_status_Mutex);
				server_status = 1;
				printf("サーバ停止要求を受信しました\n");
			}

			// パイプクライアントに応答メッセージを送信
			DWORD NumBytesWritten;
			strcpy(buf, "OK");
			bRet = WriteFile(hPipe, buf, (DWORD)strlen(buf) + 1,
				&NumBytesWritten, (LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				printf("WriteFile error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}

			// クライアントとのパイプを切断
			DisconnectNamedPipe(hPipe);

			// 新たなパイプクライアントからの接続を待つ
			bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
			if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {
				printf("ConnectNamedPipe error. (%ld)\n", GetLastError());
				break;
			}

			continue;
		}

		//イベント調査&イベントハンドルのリセット。これを発行しないとイベントリセットされないので常にイベントが発生している事になる
		WSANETWORKEVENTS mainEvents;
		if (WSAEnumNetworkEvents(srcSocket, mainHandle, &mainEvents) == SOCKET_ERROR)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			break;
		}

		//Acceptフラグを見ておく
		if (mainEvents.lNetworkEvents & FD_ACCEPT)
		{
			// クライアントから新規接続を検知
			acceptHandler(*pSocketMap, mainHandle, tp);
		}
	}

	///////////////////////////////////////////////////////////////////////
	//定期的にワーカースレッド数(ハンドラ数)をカウントしにいく
	//pSocketMap->getCount() = パイプ用(1個) + listen用(1個) + クライアント応対用
	///////////////////////////////////////////////////////////////////////

	//パイプと、それに関連つけたイベントクローズ&Mapから削除
	DisconnectNamedPipe(hPipe);
	pSocketMap->deleteSocket(eventConnect);

	//listenソケットと、それに関連づけたイベントクローズ&Mapから削除
	pSocketMap->deleteSocket(hEvent);

	//スレッド数カウント一定回数定期実行
	int count = 10;
	int duration = 2000;
	int worker_threadNum;

	while (1) {
		worker_threadNum = pSocketMap->getCount();
		if (worker_threadNum == 0) {
			printf("ワーカースレッド残0。終了に向かいます\n");
			break;
		}

		count--;

		if (count <= 0) {
			printf("タイムアップ。ワーカースレッド残%d。強制終了します\n", worker_threadNum);
			tp.terminateAllThreads();
			break;
		}

		Sleep(duration);
	}
	
	//ソケットマップ開放
	delete pSocketMap;

	//Winsock終了処理
	WSACleanup();

	printf("終了しました\n");
	//プログラムがプロセス正常終了して呼び出し元(mainなのでOS)に戻す所で不具合が起きてるっぽい
	exit(0);//とりあえず強制的に関数を終わらせる

	//return(0);
}