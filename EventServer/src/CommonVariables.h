#pragma once
//#include <stdio.h>
//#pragma comment(lib, "ws2_32.lib")
//#pragma warning(disable:4996)
//#include <WinSock2.h>
//#include <map>
//#include <thread>

//#include <windows.h>
#include "CSocketMap.h"

///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
#if defined __MAIN_SRC__
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
//bool main_thread_flag = true;
HANDLE	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	4					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
CSocketMap* pSocketMap;

#else
extern int server_status;//サーバーステータス(0:起動, 1:シャットダウン)
//bool main_thread_flag = true;
extern HANDLE	server_status_Mutex;
extern HANDLE socketMap_Mutex;
extern const char* PIPE_NAME;
#define CLIENT_MAX	4					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
extern CSocketMap* pSocketMap;
#endif