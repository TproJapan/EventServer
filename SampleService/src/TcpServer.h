#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;

class TcpServer {
public:
	int isInisialized;
	//int nRet;
	boost::asio::io_service io_service;
	thread_pool tp;

	char pipeName[80];	// パイプ名
	HANDLE hPipe;// 名前付きパイプ
	OVERLAPPED overlappedConnect;// OVARLAPPED構造体
	HANDLE eventConnect;

	//BOOL bRet;
	int nPortNo;
	WSADATA	WsaData;
	WORD wVersionRequested;
	SOCKET srcSocket;// ソケット(listen用)
	HANDLE hEvent;

	struct sockaddr_in srcAddr;
public:
	TcpServer();
	~TcpServer();
	int Func();
	int Run();
	int Terminate();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
};

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#else
extern HANDLE TcpServerMainEnd;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#endif
