﻿///////////////////////////////////////////////////////////////////////////////
// WinSockを使用したTCPサーバー
// Boost.Asioでスレッドプール
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include "BoostLog.h"
#include <boost/asio.hpp>
#include "ThreadPool.h"
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

connectClientVector connectclient_vec;

#ifndef _WIN64
#define SOCKET_ERROR	(-1)
#define INVALID_SOCKET	(-1)
#define WSAGetLastError()	errno
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
		//writeLog(5, "CreateNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "CreateNamedPipe error.\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}

	memset(&overlappedConnect, 0, sizeof(overlappedConnect));
	eventConnect = CreateEvent(0, FALSE, FALSE, 0);
	if (eventConnect == INVALID_HANDLE_VALUE) {
		//writeLog(5, "CreateNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "CreateNamedPipe error.\n", __FILENAME__, __LINE__, __func__);
		throw - 2;
	}

	overlappedConnect.hEvent = eventConnect;

	// パイプクライアントからの接続を待つ。
	// ただしOVERLAP指定なのでクライアントからの接続が無くても正常終了する
	// (実際の接続確認はWSAWaitForMultipleEvents, GetOverlappedResultで行う)
	bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
	if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {
		//writeLog(5, "ConnectNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "ConnectNamedPipe error.\n", __FILENAME__, __LINE__, __func__);
		throw - 3;
	}

	// WINSOCKの初期化(これやらないとWinSock2.hの内容が使えない)
	wVersionRequested = MAKEWORD(2, 0);
	WSADATA	WsaData;
	if (WSAStartup(wVersionRequested, &WsaData) != 0) {
		//writeLog(5, "WSAStartup() error. code=%d, %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAStartup() error.\n", __FILENAME__, __LINE__, __func__);
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
	writeLog(2, "SigAction:before sigemptyset, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	nRet = sigemptyset(&sigset);
	if (nRet != 0) throw - 1;

	//Control-C(SIGINT)で割り込まれないようにする
	writeLog(2, "SigAction:before sigaddset, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	nRet = sigaddset(&sigset, SIGINT);
	if (nRet != 0) throw - 1;
	act.sa_mask = sigset;

	writeLog(2, "SigAction:Before sigaction, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	///////////////////////////////////
	// SIGALRM捕捉
	///////////////////////////////////
	//第1引数はシステムコール番号
	//第2引数は第1引数で指定したシステムコールで呼び出したいアクション
	//第3引数は第1引数で指定したシステムコールでこれまで呼び出されていたアクションが格納される。NULLだとこれまでの動作が破棄される
	nRet = sigaction(SIGALRM, &act, NULL);
	if (nRet == -1) {
		//writeLog(4, "sigaction(sigalrm) error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(4, "%ld:%s, %s %d %s\n", errno, "sigaction(sigalrm) error.\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}

	///////////////////////////////////
	// SIGUSR2捕捉
	///////////////////////////////////
	memset(&act, 0, sizeof(act));//再度初期化
	act.sa_handler = sigusr2_handler;
	nRet = sigaction(SIGUSR2, &act, NULL);
	if (nRet == -1) {
		//writeLog(4, "sigaction(sigalrm2) error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(4, "%ld:%s, %s %d %s\n", errno, "sigaction(sigalrm2) error.\n", __FILENAME__, __LINE__, __func__);
		throw - 1;
	}
#endif

	// ポート番号の設定
	//nPortNo = 5000;
	nPortNo = portNo;

	// ソケットの生成(listen用)
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srcSocket == -1) {
		//writeLog(5, "socket error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
#ifdef _WIN64
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError, "socket error.\n", __FILENAME__, __LINE__, __func__);
#else
		writeLog(5, "%ld:%s, %s %d %s\n", errno, "socket error.\n", __FILENAME__, __LINE__, __func__);
#endif
		throw - 5;
	}


#ifdef _WIN64
	hEvent = WSACreateEvent();
	if (hEvent == INVALID_HANDLE_VALUE) {
		//writeLog(5, "WSACreateEvent error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError, "WSACreateEvent error.\n", __FILENAME__, __LINE__, __func__);
		throw - 6;
	}

	// UnixのSelectのような意味合いではない。該当ソケットはどのイベントにのみ反応するのかを定義する関数。
	// クライアントからの接続待ちソケットなのでFD_ACCEPTのみ関連付ける
	nRet = WSAEventSelect(srcSocket, hEvent, FD_ACCEPT);
	if (nRet == SOCKET_ERROR)
	{
		//writeLog(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError, "WSAEventSelect error.\n", __FILENAME__, __LINE__, __func__);
		WSACleanup();
		throw - 7;
	}
#else
	const int on = 1;
	nRet = setsockopt(srcSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (nRet == -1) {
		//writeLog(4, "setsockopt error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(4, "%ld:%s, %s %d %s\n", errno, "setsockopt error.\n", __FILENAME__, __LINE__, __func__);
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
		//writeLog(5, "bind error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "bind error.\n", __FILENAME__, __LINE__, __func__);
#else
		//writeLog(4, "bind error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", errno, "bind error.\n", __FILENAME__, __LINE__, __func__);
#endif
		throw - 8;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if (nRet == SOCKET_ERROR) {
		//writeLog(4, "listen error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
#ifdef _WIN64
		writeLog(4, "%ld:%s, %s %d %s\n", WSAGetLastError(), "listen error.\n", __FILENAME__, __LINE__, __func__);
#else
		writeLog(4, "%ld:%s, %s %d %s\n", errno, "listen error.\n", __FILENAME__, __LINE__, __func__);
#endif
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
			writeLog(2, "connectclient_vecis empty.Client Vector:%d %s %d %s\n", connectclient_vec.size(), __FILENAME__, __LINE__, __func__);
			break;
		}

		count--;

		if (count <= 0) {
			writeLog(2, "TimeUp. Finish Forcely. Client Vector:%d %s %d %s\n", connectclient_vec.size(), __FILENAME__, __LINE__, __func__);
			break;
		}

		Sleep(duration);
	}

	//Winsock終了処理
	WSACleanup();

	//printf("終了しました\n");
	writeLog(2, "Tcpサーバー停止処理を終了しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

	//サービス側に終了を知らせる
	SetEvent(TcpServerMainEnd);

#else
	int nRet = 0;

	// 接続待ちソケットのクローズ
	nRet = close(srcSocket);
	if (nRet == -1) {
		//writeLog(4, "close error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		writeLog(4, "%ld:%s, %s %d %s\n", errno, "close error.\n", __FILENAME__, __LINE__, __func__);
	}

	setServerStatus(1);

	//sigalerm発行
	alarm(60);
	writeLog(2, "alarmがセットされました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

	int past_seconds = 0;

	while (1) {
		sleep(2);
		past_seconds += 2;
		writeLog(2, "%d秒経過しました, %s %d %s\n", past_seconds, __FILENAME__, __LINE__, __func__);

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);
		bool result = connectclient_vec.empty();
		if (result == true) {
			writeLog(2, "Exit loop. Client Vector:%d %s %d %s\n", connectclient_vec.size(), __FILENAME__, __LINE__, __func__);
			break;
		}
	}

	writeLog(2, "正常終了します, %s %d %s\n", __FILENAME__, __LINE__, __func__);

#endif
}

#ifndef _WIN64
bool main_thread_flag = true;


void TcpServer::sigalrm_handler(int signo, ThreadPool _tp)
{
	writeLog(2, "sig_handler started. signo=%d, %s %d %s\n", signo, __FILENAME__, __LINE__, __func__);
	//ワーカースレッド強制終了します
	_tp.terminateAllThreads();
	writeLog(2, "ワーカースレッド強制終了しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	return;
}

void TcpServer::sigusr2_handler(int signo) {
	writeLog(2, "sig_handler started. signo=%d, %s %d %s\n", signo, __FILENAME__, __LINE__, __func__);
	main_thread_flag = false;
	writeLog(2, "main_thread_flagを書き換えました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
	return;
}

int TcpServer::Func() {
	int nRet = 0;

	while (main_thread_flag) {
		//int dstSocket = -1;		// クライアントとの通信ソケット

		// Vectorのゴミ掃除
		cleanupConnectClientVec(connectclient_vec);

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
		writeLog(2, "新規接続とクライアントから書き込みを待っています, %s %d %s\n", __FILENAME__, __LINE__, __func__);

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
				//writeLog(4, "select error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				writeLog(4, "%ld:%s, %s %d %s\n", errno, "select error.\n", __FILENAME__, __LINE__, __func__);
				exit(1);
			}
		}
		else if (nRet == 0) {
			writeLog(2, "selectでタイムアウト発生, %s %d %s\n", __FILENAME__, __LINE__, __func__);
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

int TcpServer::cleanupConnectClientVec(connectClientVector& vec)
{
	int deleteCount = 0;
	writeLog(2, "*** ゴミ掃除開始 ***.Client Vector Size:%d, %s %d %s\n", vec.size(), __FILENAME__, __LINE__, __func__);

	auto it = vec.begin();
	while (it != vec.end()) {
		std::lock_guard<std::mutex> lk((*it)->mMutex);
		writeLog(2, "*** h=%p, flag=%d, %s %d %s\n", *it, (*it)->live, __FILENAME__, __LINE__, __func__);
		if ((*it)->live == false) {
			writeLog(2, "*** h=%p deleted, %s %d %s\n", *it, __FILENAME__, __LINE__, __func__);
			delete* it;
			it = vec.erase(it);
			deleteCount++;
		}
		else {
			it++;
		}
	}

	writeLog(2, "*** deleteCount = %d ***, %s %d %s\n", deleteCount, __FILENAME__, __LINE__, __func__);
	return deleteCount;
}
#else

int TcpServer::Func() {
	int nRet;

	while (1) {
		if (getServerStatus() == 1) {
			writeLog(2, "メインループを抜けます., %s %d %s\n", __FILENAME__, __LINE__, __func__);
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
		writeLog(2, "新規接続を待っています., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		DWORD dwTimeout = TIMEOUT_MSEC;

		nRet = WSAWaitForMultipleEvents(mainEventNum, eventList, FALSE, dwTimeout, FALSE);
		if (nRet == WSA_WAIT_FAILED)
		{
			//writeLog(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAWaitForMultipleEvents error.\n", __FILENAME__, __LINE__, __func__);
			break;
		}

		if (nRet == WSA_WAIT_TIMEOUT) {
			writeLog(2, "タイムアウト発生!!!, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		writeLog(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", nRet, __FILENAME__, __LINE__, __func__);

		// イベントを検知したHANDLE
		HANDLE mainHandle = eventList[nRet];

		//ハンドル書き込み検出
		if (mainHandle == eventConnect) {
			writeLog(2, "パイプに接続要求を受けました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

			DWORD byteTransfer;
			DWORD NumBytesRead;
			DWORD dwRet = 0;
			BOOL bRet;

			bRet = GetOverlappedResult(mainHandle, &overlappedConnect, &byteTransfer, TRUE);
			writeLog(2, "GetOverlappedResult bRet = %ld, %s %d %s\n", bRet, __FILENAME__, __LINE__, __func__);

			if (bRet != TRUE) {
				//writeLog(5, "GetOverlappedResult error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "GetOverlappedResult error.\n", __FILENAME__, __LINE__, __func__);
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
				//writeLog(5, "ReadFile error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "ReadFile error.\n", __FILENAME__, __LINE__, __func__);
				break;
			}

			writeLog(2, "パイプクライアントと接続しました:[%s], %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);

			// 受信メッセージが "stop" の場合はTcpServerを停止
			if (strcmp(buf, "stop") == 0) {
				std::lock_guard<std::mutex> lk(serverStatusMutex);
				serverStatus = 1;
				writeLog(2, "サーバ停止要求を受信しました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			}

			// パイプクライアントに応答メッセージを送信
			DWORD NumBytesWritten;
			strcpy(buf, "OK");
			bRet = WriteFile(hPipe, buf, (DWORD)strlen(buf) + 1,
				&NumBytesWritten, (LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				//writeLog(5, "WriteFile error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "WriteFile error.\n", __FILENAME__, __LINE__, __func__);
				break;
			}

			// クライアントとのパイプを切断
			DisconnectNamedPipe(hPipe);

			// 新たなパイプクライアントからの接続を待つ
			bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
			if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {
				//writeLog(5, "ConnectNamedPipe error. (%ld), %s %d %s\n", GetLastError(), __FILENAME__, __LINE__, __func__);
				writeLog(5, "%ld:%s, %s %d %s\n", GetLastError(), "ConnectNamedPipe error.\n", __FILENAME__, __LINE__, __func__);
				break;
			}

			continue;
		}

		//イベント調査&イベントハンドルのリセット。これを発行しないとイベントリセットされないので常にイベントが発生している事になる
		WSANETWORKEVENTS mainEvents;
		if (WSAEnumNetworkEvents(srcSocket, mainHandle, &mainEvents) == SOCKET_ERROR)
		{
			//writeLog(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAWaitForMultipleEvents error.\n", __FILENAME__, __LINE__, __func__);
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
	std::lock_guard<std::mutex> lk(serverStatusMutex);
	serverStatus = 1;
}

///////////////////////////////////////////////////////////////////////////////
// ソケット切断済みのconnect_clientの解放処理
///////////////////////////////////////////////////////////////////////////////
int TcpServer::cleanupConnectClientVec(connectClientVector& vec)
{
	int deleteCount = 0;
	writeLog(2, "*** ゴミ掃除開始 ***, %s %d %s", __FILENAME__, __LINE__, __func__);

	auto it = vec.begin();
	while (it != vec.end()) {
		bool liveFlag = true;
		{
			// lock_guradeがインスタンスのm_mutexを参照していると
			// deleteで死んでしまうので、(*it)->_liveの内容を
			// liveFlagにコピーしておく。
			std::lock_guard<std::mutex> lk((*it)->mMutex);
			liveFlag = (*it)->live;
		}

		writeLog(2, "*** h=%p, flag=%d, %s %d %s", *it, (*it)->live, __FILENAME__, __LINE__, __func__);
		if (liveFlag == false) {
			delete* it;
			it = vec.erase(it);
			deleteCount++;
		}
		else {
			it++;
		}
	}

	writeLog(2, "deleteCount = %d ***, %s %d %s", deleteCount, __FILENAME__, __LINE__, __func__);
	return deleteCount;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool TcpServer::acceptHandler(SOCKET& sock, ThreadPool& tp)
{
	writeLog(2, "クライアント接続要求を受け付けました %s %d %s\n", __FILENAME__, __LINE__, __func__);
	struct sockaddr_in dstAddr;
	int addrlen = sizeof(dstAddr);

	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, (socklen_t*)&addrlen);
	if (newSock == -1) {
		//writeLog(5, "accept error. (%ld) %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "accept error.\n", __FILENAME__, __LINE__, __func__);
		return false;
	}

	writeLog(2, "[%s]から接続を受けました. newSock=%d, %s %d %s\n", inet_ntoa(dstAddr.sin_addr), newSock, __FILENAME__, __LINE__, __func__);
	//プールスレッドにバインド
	ConnectClient* h = new ConnectClient(newSock);
	tp.post(boost::bind(&ConnectClient::func, h));
	connectclient_vec.push_back(h); //vectorに追加
	return true;
}