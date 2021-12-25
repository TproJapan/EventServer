#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>	
#include <string>
#include <string.h>
//#include <msgpack.hpp>

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
#define CLIENT_MAX	32000 //マシーンリソースに依存する数
//#define CLIENT_MAX	2 //マシーンリソースに依存する数
#define SELECT_TIMER_SEC	3			// selectのタイマー(秒)
#define SELECT_TIMER_USEC	0			// selectのタイマー(マイクロ秒)

////強制起動オプション用
//#define TXT_SERVER_PID  "/tmp/EventServer.txt"
//#define TXT_LOG_PID "/tmp/Log.txt"
//#define TXT_OBSERVER_PID "/tmp/Observer.txt"

//enum class EDebugLv{
//	__NOTICE,
//	__DEBUG,
//	__INFO,
//	__ERROR
//};//

//extern EDebugLv CurrentLogLv;//

////ログ用ヘッダー
//using LogStruct = struct {
//	timespec  now;
//	pid_t pid;
//	//char* allocBuf;
//	char allocBuf[1024];//文字数は要検討
//};//

////client - server 送受信データのヘッダー
//const int _blockSize = 10;
//const int MaxBufSize = 1024;//

////通常typedefは定義のみで値の代入は行わない。
////最近のgccは定義&代入を許しているが昔のgccはコンパイルエラーになる
//using head =  struct {
//	int totalSize;//メッセージ部分の文字列バイト数
//	int blockSize = _blockSize;
//	int recordNo;
//};//

//using myVector3 = struct {
//	float _x;
//	float _y;
//	float _z;
//	//シリアライズしたい変数を設定
//	MSGPACK_DEFINE(_x, _y, _z);
//};//

//using MessStruct = struct {
//	head _head;
//	char _data[_blockSize];
//};//

////stdoutで標準出力してデバック
//static void printHead(head* pHead, FILE* fp) {
//	fprintf(fp,
//			 "★★ header(totalSize:%d, blockSize:%d, recordNo:%d)\n",
//			 pHead->totalSize,
//			 pHead->blockSize,
//			 pHead->recordNo);
//	char* pData = (char*)pHead + sizeof(head);
//	fprintf(fp,
//			 "★★ data:[%s]\n",
//			 pData);
//	return;
//}//

//int sendFunc(char*, int data_size, int&);
//int commonRcv(int& dstSocket, char* bufForOnce, size_t& stSize, char* bufForWholeMes, bool& flag);
//int newCommonRcv(int& dstSocket, char* bufForOnce, char* bufForWholeMes, int& data_size);
//int logFunc(int fifo, EDebugLv LogLv, pid_t pid, const char* fmt, ...);
//bool write_pid(char* buff, std::string macro);
//int DebugLogForDaemon(const char* message);
int exist_process (std::string macro);
int get_pid(std::string macro);
int GetServerStatus();
int SetServerStatus(int status);
//int ForcelyKillProcess(std::string macro);
//int err_notify(int fd_num, char* err_detail);
//int err_notify_log(int fd_num, char* err_detail, int logpipe);