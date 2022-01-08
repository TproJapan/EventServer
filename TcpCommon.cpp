#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
// #include "BoostLog.h"

#include <boost/system/error_code.hpp>


#include "TcpCommon.h"
#include <syslog.h>
#include <string>
// #include <mutex>

int server_status; //サーバーステータス(0:起動, 1:シャットダウン)
std::mutex	server_status_Mutex;

int GetServerStatus(){
	std::lock_guard<std::mutex> lock(server_status_Mutex);
	return server_status;
}

int SetServerStatus(int status){
	std::lock_guard<std::mutex> lock(server_status_Mutex);
	server_status = status;
	return server_status;
}	
//EDebugLv CurrentLogLv = EDebugLv::__DEBUG;
//void createTimeStamp(std::string& strTime);//

//int sendFunc(char* buf, int data_size, int& dstSocket){
//	//restMessByteが0以下になってsendしきったかどうか
//	bool loopFlag = true;//

//	//返信No初期化
//	int recordNo = 1;//

//	MessStruct devidedMess = MessStruct();
//	devidedMess._head.totalSize = data_size;
//	int restMessByte = data_size;//要返信バイト数//

//	while(loopFlag){
//		//devidedMess._head.totalSize = restMessByte;
//		devidedMess._head.recordNo = recordNo;//

//		//第一引数:コピー先アドレス、第二引数:コピー元アドレス、第三引数:サイズ
//		memcpy(devidedMess._data, (buf + _blockSize*(recordNo-1)), _blockSize);//

//		size_t szRet = send(dstSocket, &devidedMess, sizeof(MessStruct), 0);//

//		if( szRet == -1) return -1;//

//		//フラグとNo更新
//		restMessByte -= _blockSize;
//		if(restMessByte < 1) loopFlag = false;
//		else recordNo++;
//	}//

//	return 0;
//}//

//int newCommonRcv(int& dstSocket, char* bufForOnce, char* bufForWholeMes, int& data_size){
//	//flag:予定総受信バイト数に現受信バイト数が届いているかどうか
//	bool flag = false;
//	size_t stSize = MaxBufSize;//

//	while(flag == false){
//		size_t _stSize= recv(dstSocket,
//						bufForOnce,
//						stSize,
//						0);//

//		if ( _stSize == 0 || _stSize == -1) {
//			return -1;
//		}//

//		//受信したバッファの総レコード数
//		int nReceivedRecCount = _stSize/sizeof(MessStruct);
//		int nRecordCount = 0;//

//		//総レコード数分回す
//		while( nRecordCount < nReceivedRecCount) {     
//			//////////////////////////////////////////////////////////
//			/////受信メッセージからヘッダーを取って実メッセージ連結する処理/////
//			////////////////////////////////////////////////////////////

//			//ヘッダーの番地を特定
//			MessStruct* pDevidedMess = (MessStruct*)(bufForOnce + sizeof(MessStruct)*nRecordCount);
//			nRecordCount++;//

//			int recordNo = pDevidedMess->_head.recordNo;//

//			//第一引数:コピー先アドレス、第二引数:コピー元アドレス、第三引数:サイズ
//			memcpy(bufForWholeMes + _blockSize * (recordNo-1), pDevidedMess->_data, _blockSize);//

//			//現在の受信バイト数が総バイト数より小さければ
//			if(recordNo * _blockSize < pDevidedMess->_head.totalSize) flag = false;
//			else flag = true;//

//			//シリアライズされたバイナリデータのサイズ
//			if (flag == true) {
//				data_size = pDevidedMess->_head.totalSize;
//			}
//		}
//	}//

//	return(0);
//}//

//int logFunc(int fifo, EDebugLv LogLv, pid_t pid, const char* fmt, ...){
//	//ログレベル確認
//	if(static_cast<int>(CurrentLogLv) > static_cast<int>(LogLv))
//	{
//		return(0);
//	}//

//	//フォーマット文字列処理
//	va_list ap;
//	char* allocBuf;//連結後の文字列//

//	va_start(ap, fmt);
//	int nSize = vasprintf(&allocBuf, fmt, ap);
//	va_end(ap);//

//	//タイムスタンプ
//	timespec  now;
//	clock_gettime(CLOCK_REALTIME, &now);//

//	//ログインスタンス生成
//	LogStruct Log;//

//	//初期化
//	Log.now = now;
//	Log.pid = pid;
//	memset(&(Log.allocBuf), 0, sizeof(Log.allocBuf));//0クリア//

//	//strncpy(Log.allocBuf, allocBuf, sizeof(Log.allocBuf));/フォーマット文字数以上のメモリ領域にアクセスしてしまい落ちてしまう
//	strncpy(Log.allocBuf, allocBuf, nSize + 1);//vasprintfの戻り値は\nを含まない文字数
//	//strncpy(Log.allocBuf, allocBuf, strlen(allocBuf)+1);//strlenの戻り値は\nを含まない文字数//

//	//名前付きパイプでwrite
//    if (write(fifo, &Log, sizeof(Log)) != sizeof(Log))//writeが失敗したら
//	{
//        //perror("write()");
//		write_log(4, "write log fifo failed in TcpCommon\n");//

//		//エラーになっても解放を忘れない
//		free(allocBuf);//

//		//メイン関数へ-1を返す。メイン関数側では−１をフックにclose(fifo);
//        return -1;
//    }//

//	//正常終了でも解放
//	free(allocBuf);//

//	return(0);
//};//

//bool write_pid(char* buff, std::string macro){
//  FILE *fp;
//  fp = fopen(macro.c_str(), "w");
//  if(fp == NULL){
//	  return false;
//  }
//  fprintf(fp, "%s", buff);
//  fclose(fp);//

//  return true;
//}//

//int DebugLogForDaemon(const char* message){//

//  std::string strTimeStamp;
//  createTimeStamp(strTimeStamp);//

//  FILE *fp;
//  fp = fopen(TMP_LOGFILE, "a");
//  if(fp == NULL){
//	  return -1;
//  }
//  fprintf(fp, "[%s][%d] %s", strTimeStamp.c_str(), getpid(), message);
//  fflush(fp);
//  fclose(fp);
//  return 0;
//}//

//void createTimeStamp(std::string& strTime)
//{
//	struct timespec  now;
//	clock_gettime(CLOCK_REALTIME, &now);
//	struct tm local;
//	localtime_r(&now.tv_sec, &local);//

//	char timebuff[32];
//	sprintf(timebuff ,
//			"%04d/%02d/%02d %02d:%02d:%02d.%03ld",
//			local.tm_year + 1900,
//			local.tm_mon + 1,
//			local.tm_mday,
//			local.tm_hour,
//			local.tm_min,
//			local.tm_sec,
//			now.tv_nsec / 1000000 );
//	//printf("%s\n", timebuff);
//	strTime = timebuff;
//	return;
//}//

/////////////////////////////////////////////////////////////////////////////////
//// プロセス生存確認
/////////////////////////////////////////////////////////////////////////////////
int exist_process (std::string macro)
{
	char buff[256];
	pid_t pid = get_pid(macro);
	if(pid == -1){
		sprintf(buff, "pid file is no exist. (%s)\n", macro.c_str());
		// write_log(4, buff);
		return -1;
	}else if(pid == -2){
		sprintf(buff, "pid file is blank. (%s)\n", macro.c_str());
		// write_log(4, buff);
		return -1;		
	}
	sprintf(buff, "%d", pid);

	//プロセス生存確認
	int r = kill (pid, 0);
	if (r == 0)
	{
		// pid は存在する
		std::string message = std::string(buff);
		message += " is exist!\n";//文字列連結
		char *cstr = new char[message.length() + 1];//char領域メモリを新規作成
		strcpy(cstr, message.c_str());//コピー
		//sprintf(buff, "pid file is no exist. (%s)\n", macro.c_str());
		// write_log(4, cstr);
		delete [] cstr;//メモリ解放
		return (1);
	}

	// pid は存在しない
	std::string message = std::string(buff);
	message += " is no exist!\n";//文字列連結
	char *cstr = new char[message.length() + 1];//char領域メモリを新規作成
	strcpy(cstr, message.c_str());//コピー
	// write_log(2, cstr);
	delete [] cstr;//メモリ解放
	return (0);
}

int get_pid(std::string macro){
	char buff[256];
	pid_t pid;
	FILE *fp;

	//fopenはwモードだと既存を破棄して新規作成になってしまうのでrモードで開く
	fp = fopen(macro.c_str(), "r");
	if(fp == NULL){
		//perror("Open PID File Failed!\n");
		// write_log(4, "Open PID File Failed!\n");
		return -1;
	}
	
	char* ptr = fgets(buff, 256, fp);
	if(ptr == NULL){
		return -2;
	}else{
		char tmpbuff[1024];
		sprintf(tmpbuff,"buff = (%s)\n", buff);
		// write_log(2, tmpbuff);
	}
	pid = atoi(buff);
	fclose(fp);
	
	return pid;
}

//int ForcelyKillProcess(std::string macro){
//	//文字列整形
//	std::string mystr = macro;
//	std::string substr = mystr.substr(5);
//	substr = substr.substr(0, substr.length() - 4);
//	const char* pNameCharPtr = substr.c_str();
//	printf("Process Name: %s\n", pNameCharPtr);//

//	//psコマンド実行し結果をテキストファイルにリダイレクト
//	unlink(macro.c_str());
//	char system_buff[512];
//	sprintf(system_buff, "ps aux | grep %s | grep -v grep | awk '{ print $2 }' > %s", pNameCharPtr, macro.c_str());
//	system(system_buff);//

//	//テキストファイルを読み取る
//	FILE *fp;
//	fp = fopen(macro.c_str(), "r");
//	if(fp == NULL){
//		printf("%s does not exist\n", macro.c_str());
//		return -1;
//	}//

//	char tmpbuf[256];
//	size_t n = fread(tmpbuf,1, 256,fp);
//	fclose(fp);//

//	if(n == 0){
//		printf("%s process is not exists!\n", pNameCharPtr);
//	}else{
//		printf("%s process: %s", pNameCharPtr, tmpbuf);
//		int current_pid = atoi(tmpbuf);//

//		//現プロセスをkill
//		int result = kill(current_pid, SIGKILL);
//		if(result == -1){
//			printf("Failed to kill %s process\n", pNameCharPtr);
//			return -1;
//		}else{
//			printf("Succeeded to kill %s process\n", pNameCharPtr);
//		}
//	}//

//	return 0;
//}//

//int err_notify(int fd_num, char* err_detail){
//	write_log(4, err_detail);
//	char err_buff[16];
//	sprintf(err_buff, "err");//この時点でerr_buffにはerr¥0が書き込まれている
//	if (write(fd_num, err_buff, strlen(err_buff) + 1) != strlen(err_buff) + 1)//これだとerr¥0まで書き込まれる
//	{
//		write_log(4, "open fd failed in err_notify()\n");
//		close(fd_num);
//		exit(-1);
//	}//

//	exit(0);
//}//

//int err_notify_log(int fd_num, char* err_detail, int logpipe){
//	write_log(4, err_detail);
//	char err_buff[16];
//	sprintf(err_buff, "err");//この時点でerr_buffにはerr¥0が書き込まれている
//	if (write(fd_num, err_buff, strlen(err_buff) + 1) != strlen(err_buff) + 1)//これだとerr¥0まで書き込まれる
//	{
//		write_log(4, "open fd failed in err_notify_log()\n");
//		close(fd_num);
//		close(logpipe);
//		exit(-1);
//	}//

//	close(fd_num);
//	close(logpipe);//

//	exit(0);
//}