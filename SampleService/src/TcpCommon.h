#pragma once
#ifndef _WIN64
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>	
#include <string>
#include <string.h>
#include <mutex>
#else
#include "ThreadPool.h"
#include <vector>
#endif

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
int getServerStatus();

#ifndef _WIN64
#define PIPE_START "/tmp/fifo_start"//Startに完了報告する名前付きパイプ
#define PIPE_STOP "/tmp/fifo_stop"//Stopに完了報告する名前付きパイプ
#define REQUEST_PIPE "/tmp/request_pipe"
#define PID_SERVER "/tmp/myServer.pid"
#define PID_LOG "/tmp/myLog.pid"
#define PID_OBSERVER "/tmp/Observer.pid"
#define TMP_LOGFILE "/tmp/log"
#define QUE_NAME "/mq_stop_observer"
// #define PROJ_HOME "/home/tateda/EventServer/SampleService/src"
#define CLIENT_MAX	800 //マシーンリソースに依存する数
#define SELECT_TIMER_SEC	30			// selectのタイマー(秒)
#define SELECT_TIMER_USEC	0			// selectのタイマー(マイクロ秒)
int setServerStatus(int status);
#if defined __TCPCOMMON__
int serverStatus = 0; //サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	serverStatusMutex;
#else
extern int serverStatus;
extern std::mutex	serverStatusMutex;
#endif
#else
#define CLIENT_MAX	400					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
#if defined __MAIN_SRC__
int serverStatus = 0;//サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	serverStatusMutex;
HANDLE socketMapMutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#else
extern int serverStatus;//サーバーステータス(0:起動, 1:シャットダウン)
extern std::mutex	serverStatusMutex;
extern HANDLE socketMapMutex;
extern const char* PIPE_NAME;
#endif
#endif