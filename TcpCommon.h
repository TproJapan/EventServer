#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>	
#include <string>
#include <string.h>

#define PIPE_START "/tmp/fifo_start"//Startに完了報告する名前付きパイプ
#define PIPE_STOP "/tmp/fifo_stop"//Stopに完了報告する名前付きパイプ

//EventServerが自ら書き込み&読み込みする
//ObserverからEventServerプロセスにLogプロセス復旧要求を書き込む
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

int GetServerStatus();
int SetServerStatus(int status);