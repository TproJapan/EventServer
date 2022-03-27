#pragma once
#ifdef _WIN64
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#endif

#include <vector>
#define __TCP_SERVER__
#include "TcpCommon.h"
#include "ConnectClient.h"
#ifdef _WIN64
// Windows専用ヘッダー

#else
// Unix専用ヘッダー

#endif

typedef std::vector<ConnectClient*> connectclient_vector;


class TcpServer {
public:
#ifdef _WIN64
	int isInisialized;
	HANDLE hPipe;// 名前付きパイプ
	OVERLAPPED overlappedConnect;// OVARLAPPED構造体
	HANDLE eventConnect;
	WORD wVersionRequested;
	SOCKET srcSocket;// ソケット(listen用)
	HANDLE hEvent;
#else
	int srcSocket;
#endif
	boost::asio::io_service io_service;
	thread_pool tp;
	int nPortNo;
	struct sockaddr_in srcAddr;

public:
	TcpServer(int portNo);
	~TcpServer();
	int Func();
	int Terminate();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
#ifdef _WIN64
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
#else
	static void sigalrm_handler(int signo, thread_pool _tp);
	static void sigusr2_handler(int signo);
#endif
};

#ifdef _WIN64
#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#else
extern HANDLE TcpServerMainEnd;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#endif

#endif