///////////////////////////////////////////////////////////////////////////////
// WinSockを使用したTCPサーバー
// Boost.Asioでスレッドプール
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include "BoostLog.h"
#include <boost/asio.hpp>
#include "thread_pool.h"
#include "ConnectClient.h"

#ifdef _WIN64
#define __MAIN_SRC__
#endif

#include "TcpServer.h"
#include "TcpCommon.h"

#ifdef _WIN64
#include <tchar.h>
#else
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
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
#endif

connectclient_vector connectclient_vec;

#ifndef _WIN64
#define SOCKET_ERROR	(-1)
#endif

TcpServer::TcpServer(int portNo) :tp(io_service, CLIENT_MAX)
{
#ifdef _WIN64
	int nRet;
	bool bRet;

	// パイプ名の組み立て
	char pipeName[80];	// パイプ名
	wsprintf((LPWSTR)pipeName, (LPCWSTR)PIPE_NAME, ".");

	hPipe = CreateNamedPipe((LPCWSTR)_T("\\\\.\\pipe\\EventServer"),		// パイプ名
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,		// パイプのアクセスモード
		PIPE_TYPE_MESSAGE,		// パイプの種類, 待機モード
		1,		// インスタンスの最大数
		0,		// 出力バッファのサイズ
		0,		// 入力バッファのサイズ
		150,	// タイムアウト値
		(LPSECURITY_ATTRIBUTES)NULL);	// セキュリティ属性

	if (hPipe == INVALID_HANDLE_VALUE) {
		write_log(5, "CreateNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		throw - 1;
	}

	memset(&overlappedConnect, 0, sizeof(overlappedConnect));
	eventConnect = CreateEvent(0, FALSE, FALSE, 0);
	if (eventConnect == INVALID_HANDLE_VALUE) {
		write_log(5, "CreateNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		throw - 2;
	}

	overlappedConnect.hEvent = eventConnect;

	// パイプクライアントからの接続を待つ。
	// ただしOVERLAP指定なのでクライアントからの接続が無くても正常終了する
	// (実際の接続確認はWSAWaitForMultipleEvents, GetOverlappedResultで行う)
	bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
	if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {
		write_log(5, "ConnectNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		throw - 3;
	}

	// WINSOCKの初期化(これやらないとWinSock2.hの内容が使えない)
	wVersionRequested = MAKEWORD(2, 0);
	WSADATA	WsaData;
	if (WSAStartup(wVersionRequested, &WsaData) != 0) {
		write_log(5, "WSAStartup() error. code=%d, %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		throw - 4;
	}
#else
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
	write_log(4, "before sigemptyset, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	nRet = sigemptyset(&sigset);
	if (nRet != 0) throw - 1;

	//Control-C(SIGINT)で割り込まれないようにする
	write_log(4, "before sigaddset, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	nRet = sigaddset(&sigset, SIGINT);
	if (nRet != 0) throw - 1;
	act.sa_mask = sigset;

	write_log(4, "Before sigaction, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	///////////////////////////////////
	// SIGALRM捕捉
	///////////////////////////////////
	//第1引数はシステムコール番号
	//第2引数は第1引数で指定したシステムコールで呼び出したいアクション
	//第3引数は第1引数で指定したシステムコールでこれまで呼び出されていたアクションが格納される。NULLだとこれまでの動作が破棄される
	nRet = sigaction(SIGALRM, &act, NULL);
	if (nRet == -1) {
		write_log(4, "sigaction(sigalrm) error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}

	///////////////////////////////////
	// SIGUSR2捕捉
	///////////////////////////////////
	memset(&act, 0, sizeof(act));//再度初期化
	act.sa_handler = sigusr2_handler;
	nRet = sigaction(SIGUSR2, &act, NULL);
	if (nRet == -1) {
		write_log(4, "sigaction(sigalrm2) error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}
#endif

	// ポート番号の設定
	//nPortNo = 5000;
	nPortNo = portNo;

	// ソケットの生成(listen用)
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srcSocket == -1) {
		write_log(5, "socket error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 5;
	}


#ifdef _WIN64
	hEvent = WSACreateEvent();
	if (hEvent == INVALID_HANDLE_VALUE) {
		write_log(5, "WSACreateEvent error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 6;
	}

	// UnixのSelectのような意味合いではない。該当ソケットはどのイベントにのみ反応するのかを定義する関数。
	// クライアントからの接続待ちソケットなのでFD_ACCEPTのみ関連付ける
	nRet = WSAEventSelect(srcSocket, hEvent, FD_ACCEPT);
	if (nRet == SOCKET_ERROR)
	{
		write_log(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		WSACleanup();
		throw - 7;
	}
#else
	const int on = 1;
	nRet = setsockopt(srcSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (nRet == -1) {
		write_log(4, "setsockopt error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}
#endif

	///////////////////////////////////
	// socketの設定
	///////////////////////////////////
	// listen用sockaddrの設定
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(nPortNo);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	// ソケットのバインド
	nRet = bind(srcSocket, (struct sockaddr*)&srcAddr, sizeof(srcAddr));
	if (nRet == SOCKET_ERROR) {
#ifdef _WIN64
		write_log(5, "bind error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
#else
		write_log(4, "bind error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
#endif
		throw - 8;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if (nRet == SOCKET_ERROR) {
		write_log(4, "listen error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}
}

TcpServer::~TcpServer()
{
#ifdef _WIN64
	//パイプと、それに関連つけたイベントクローズ
	DisconnectNamedPipe(hPipe);
	CloseHandle(eventConnect);

	//listenソケットと、それに関連づけたイベントクローズ
	closesocket(srcSocket);
	CloseHandle(hEvent);

	//スレッド数カウント一定回数定期実行
	int count = 10;
	int duration = 2000;
	int worker_threadNum;

	while (1) {
		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);
		bool result = connectclient_vec.empty();

		if (result == true) {
			write_log(2, "connectclient_vecがempty。ループを抜けます, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			break;
		}

		count--;

		if (count <= 0) {
			write_log(2, "タイムアップ。connectclient_vec残%d。強制終了します%s %d %s\n", connectclient_vec.size(), __FILENAME__, __LINE__, __func__);
			break;
		}

		Sleep(duration);
	}

	//Winsock終了処理
	WSACleanup();

	//printf("終了しました\n");
	write_log(2, "Tcpサーバー停止処理を終了しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

	//サービス側に終了を知らせる
	SetEvent(TcpServerMainEnd);

#else
	int nRet = 0;

	// 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if (nRet == -1) {
		write_log(4, "close error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	}

	SetServerStatus(1);

	//sigalerm発行
	alarm(60);
	write_log(2, "alarmがセットされました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

	int past_seconds = 0;

	while (1) {
		sleep(2);
		past_seconds += 2;
		write_log(2, "%d秒経過しました, %s %d %s\n", past_seconds, __FILENAME__, __LINE__, __func__);

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);
		bool result = connectclient_vec.empty();
		if (result == true) {
			write_log(2, "ループを抜けます, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			break;
		}
	}

	write_log(2, "正常終了します, %s %d %s\n", __FILENAME__, __LINE__, __func__);

#endif
}

#ifndef _WIN64
bool main_thread_flag = true;


void TcpServer::sigalrm_handler(int signo, thread_pool _tp)
{
	write_log(2, "sig_handler started. signo=%d, %s %d %s\n", signo, __FILENAME__, __LINE__, __func__);
	//ワーカースレッド強制終了します
	_tp.terminateAllThreads();
	write_log(2, "ワーカースレッド強制終了しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	return;
}

void TcpServer::sigusr2_handler(int signo) {
	write_log(2, "sig_handler started. signo=%d, %s %d %s\n", signo, __FILENAME__, __LINE__, __func__);
	main_thread_flag = false;
	write_log(2, "main_thread_flagを書き換えました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	return;
}

int TcpServer::Func() {
	int nRet = 0;

	while (main_thread_flag) {
		//int dstSocket = -1;		// クライアントとの通信ソケット

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
		tval.tv_sec = SELECT_TIMER_SEC;	// time_t  秒
		tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒

		//printf("新規接続とクライアントから書き込みを待っています.\n");	
		write_log(2, "新規接続とクライアントから書き込みを待っています, %s %d %s\n", __FILENAME__, __LINE__, __func__);

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
			&tval);

		if (nRet == -1) {
			if (errno == EINTR) {//シグナル割り込みは除外
				continue;
			}
			else {
				// selectが異常終了
				write_log(4, "select error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				exit(1);
			}
		}
		else if (nRet == 0) {
			write_log(2, "selectでタイムアウト発生, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		///////////////////////////////////////
		// 反応のあったソケットをチェック
		///////////////////////////////////////

		//TODO 現在接続数がCLIANT_MAX数より少ないかチェック
		//多かったら新規接続を受け付けない
		 // 新規のクライアントから接続要求がきた
		if (FD_ISSET(srcSocket, &readfds)) {
			acceptHandler(srcSocket, tp);// クライアントから新規接続を検知
		}
	}

	return(0);
}

int TcpServer::cleanupConnectClientVec(connectclient_vector& vec)
{
	int deleteCount = 0;
	write_log(2, "*** ゴミ掃除開始 ***, %s %d %s\n", __FILENAME__, __LINE__, __func__);

	auto it = vec.begin();
	while (it != vec.end()) {
		std::lock_guard<std::mutex> lk((*it)->m_mutex);
		write_log(2, "*** h=%p, flag=%d, %s %d %s\n", *it, (*it)->_live, __FILENAME__, __LINE__, __func__);
		if ((*it)->_live == false) {
			write_log(2, "*** h=%p deleted, %s %d %s\n", *it, __FILENAME__, __LINE__, __func__);
			delete* it;
			it = vec.erase(it);
			deleteCount++;
		}
		else {
			it++;
		}
	}

	write_log(2, "*** deleteCount = %d ***, %s %d %s\n", deleteCount, __FILENAME__, __LINE__, __func__);
	return deleteCount;
}
#else

int TcpServer::Func() {
	int nRet;

	while (1) {
		if (GetServerStatus() == 1) {
			write_log(2, "メインループを抜けます., %s %d %s\n", __FILENAME__, __LINE__, __func__);
			break;
		}

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);

		//イベントを配列で管理
		const int mainEventNum = 2;
		HANDLE eventList[mainEventNum];
		eventList[0] = hEvent;//listenイベントのみ
		eventList[1] = eventConnect;//名前付きパイプ

		//printf("新規接続を待っています.\n");
		write_log(2, "新規接続を待っています., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		DWORD dwTimeout = TIMEOUT_MSEC;

		nRet = WSAWaitForMultipleEvents(mainEventNum, eventList, FALSE, dwTimeout, FALSE);
		if (nRet == WSA_WAIT_FAILED)
		{
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		if (nRet == WSA_WAIT_TIMEOUT) {
			write_log(2, "タイムアウト発生!!!, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		write_log(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", nRet, __FILENAME__, __LINE__, __func__);

		// イベントを検知したHANDLE
		HANDLE mainHandle = eventList[nRet];

		//ハンドル書き込み検出
		if (mainHandle == eventConnect) {
			write_log(2, "パイプに接続要求を受けました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

			DWORD byteTransfer;
			DWORD NumBytesRead;
			DWORD dwRet = 0;
			BOOL bRet;

			bRet = GetOverlappedResult(mainHandle, &overlappedConnect, &byteTransfer, TRUE);
			write_log(2, "GetOverlappedResult bRet = %ld, %s %d %s\n", bRet, __FILENAME__, __LINE__, __func__);

			if (bRet != TRUE) {
				write_log(5, "GetOverlappedResult error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
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
				write_log(5, "ReadFile error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				break;
			}

			write_log(2, "パイプクライアントと接続しました:[%s], %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);

			// 受信メッセージが "stop" の場合はTcpServerを停止
			if (strcmp(buf, "stop") == 0) {
				std::lock_guard<std::mutex> lk(server_status_Mutex);
				server_status = 1;
				write_log(2, "サーバ停止要求を受信しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			}

			// パイプクライアントに応答メッセージを送信
			DWORD NumBytesWritten;
			strcpy(buf, "OK");
			bRet = WriteFile(hPipe, buf, (DWORD)strlen(buf) + 1,
				&NumBytesWritten, (LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				write_log(5, "WriteFile error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				break;
			}

			// クライアントとのパイプを切断
			DisconnectNamedPipe(hPipe);

			// 新たなパイプクライアントからの接続を待つ
			bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
			if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {
				write_log(5, "ConnectNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
				break;
			}

			continue;
		}

		//イベント調査&イベントハンドルのリセット。これを発行しないとイベントリセットされないので常にイベントが発生している事になる
		WSANETWORKEVENTS mainEvents;
		if (WSAEnumNetworkEvents(srcSocket, mainHandle, &mainEvents) == SOCKET_ERROR)
		{
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		//Acceptフラグを見ておく
		if (mainEvents.lNetworkEvents & FD_ACCEPT)
		{
			// クライアントから新規接続を検知
			acceptHandler(srcSocket, tp);
		}
	}

	return(0);
}

void TcpServer::StopTcpServer()
{
	std::lock_guard<std::mutex> lk(server_status_Mutex);
	server_status = 1;
}

///////////////////////////////////////////////////////////////////////////////
// ソケット切断済みのconnect_clientの解放処理
///////////////////////////////////////////////////////////////////////////////
int TcpServer::cleanupConnectClientVec(connectclient_vector& vec)
{
	int deleteCount = 0;
	write_log(2, "*** ゴミ掃除開始 ***, %s %d %s", __FILENAME__, __LINE__, __func__);

	auto it = vec.begin();
	while (it != vec.end()) {
		bool liveFlag = true;
		{
			// lock_guradeがインスタンスのm_mutexを参照していると
			// deleteで死んでしまうので、(*it)->_liveの内容を
			// liveFlagにコピーしておく。
			std::lock_guard<std::mutex> lk((*it)->m_mutex);
			liveFlag = (*it)->_live;
		}

		write_log(2, "*** h=%p, flag=%d, %s %d %s", *it, (*it)->_live, __FILENAME__, __LINE__, __func__);
		if (liveFlag == false) {
			delete* it;
			it = vec.erase(it);
			deleteCount++;
		}
		else {
			it++;
		}
	}

	write_log(2, "konishi *** deleteCount = %d ***, %s %d %s", deleteCount, __FILENAME__, __LINE__, __func__);
	return deleteCount;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool TcpServer::acceptHandler(SOCKET& sock, thread_pool& tp)
{
	write_log(2, "クライアント接続要求を受け付けました %s %d %s\n", __FILENAME__, __LINE__, __func__);
	struct sockaddr_in dstAddr;
	int addrlen = sizeof(dstAddr);
#ifdef _WIN64
	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		write_log(5, "accept error. (%ld) %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		return false;
	}
#else
	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, (socklen_t*)&addrlen);
	if (newSock == -1) {
		write_log(4, "accept error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		return false;
	}
#endif
	write_log(2, "[%s]から接続を受けました. newSock=%d, %s %d %s\n", inet_ntoa(dstAddr.sin_addr), newSock, __FILENAME__, __LINE__, __func__);
	//プールスレッドにバインド
	ConnectClient* h = new ConnectClient(newSock);
	tp.post(boost::bind(&ConnectClient::func, h));
	connectclient_vec.push_back(h); //vectorに追加
	return true;
}