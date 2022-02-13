#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "ConnectClient.h"
#include "TcpCommon.h"

int Tcpserver();
void StopTcpServer();//サーバーステータス(0:起動, 1:シャットダウン)		
int cleanupConnectClientVec(connectclient_vector& vec);

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#else
extern HANDLE TcpServerMainEnd;// Tcpserverの終了をサービスに知らせる為に使用するイベント
#endif
