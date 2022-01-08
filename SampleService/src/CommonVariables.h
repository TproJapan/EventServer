#pragma once
#include "CSocketMap.h"
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
#if defined __MAIN_SRC__
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	2					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
CSocketMap* pSocketMap;
typedef std::vector<ConnectClient*> connectclient_vector;
connectclient_vector connectclient_vec;//new ConnectClient(CommonFunc.cpp内)解放漏れ防止の管理Vector
#else
extern int server_status;//サーバーステータス(0:起動, 1:シャットダウン)
extern std::mutex	server_status_Mutex;
extern HANDLE socketMap_Mutex;
extern const char* PIPE_NAME;
#define CLIENT_MAX	2					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
extern CSocketMap* pSocketMap;
typedef std::vector<ConnectClient*> connectclient_vector;
extern connectclient_vector connectclient_vec;//new ConnectClient(CommonFunc.cpp内)解放漏れ防止の管理Vector
#endif