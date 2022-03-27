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
#include "thread_pool.h"
#include <vector>
#endif

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
int GetServerStatus();

#ifndef _WIN64
#define PIPE_START "/tmp/fifo_start"//Startに完了報告する名前付きパイプ
#define PIPE_STOP "/tmp/fifo_stop"//Stopに完了報告する名前付きパイプ
#define REQUEST_PIPE "/tmp/request_pipe"
#define PID_SERVER "/tmp/myServer.pid"
#define PID_LOG "/tmp/myLog.pid"
#define PID_OBSERVER "/tmp/Observer.pid"
#define TMP_LOGFILE "/tmp/log"
#define QUE_NAME "/mq_stop_observer"
#define PROJ_HOME "/home/tateda/EventServer"
#define CLIENT_MAX	800 //マシーンリソースに依存する数
#define SELECT_TIMER_SEC	30			// selectのタイマー(秒)
#define SELECT_TIMER_USEC	0			// selectのタイマー(マイクロ秒)
int SetServerStatus(int status);
#if defined __TCPCOMMON__
int server_status = 0; //サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	server_status_Mutex;
#else
extern int server_status;
extern std::mutex	server_status_Mutex;
#endif
#else
#define CLIENT_MAX	400					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)
#if defined __MAIN_SRC__
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#else
extern int server_status;//サーバーステータス(0:起動, 1:シャットダウン)
extern std::mutex	server_status_Mutex;
extern HANDLE socketMap_Mutex;
extern const char* PIPE_NAME;
#endif
#endif