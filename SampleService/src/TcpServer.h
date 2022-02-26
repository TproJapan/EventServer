#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;
//int Tcpserver();
class TcpServer {
public:
	int nRet;
	boost::asio::io_service io_service;
	thread_pool* tp_ptr;

	char pipeName[80];	// パイプ名
	HANDLE hPipe;// 名前付きパイプ
public:
	TcpServer();
	~TcpServer();
	int Func();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
};

//void StopTcpServer();//サーバーステータス(0:起動, 1:シャットダウン)		
//int cleanupConnectClientVec(connectclient_vector& vec);
//bool acceptHandler(SOCKET& sock, thread_pool& tp);

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#else
extern HANDLE TcpServerMainEnd;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#endif
