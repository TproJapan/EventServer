#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "ConnectClient.h"
#include "CommonVariables.h"

int Tcpserver();
void StopTcpServer();//サーバーステータス(0:起動, 1:シャットダウン)		
int cleanupConnectClientVec(connectclient_vector& vec);
