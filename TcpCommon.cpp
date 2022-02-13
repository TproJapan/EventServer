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
#include <boost/system/error_code.hpp>
#include "TcpCommon.h"
#include <syslog.h>
#include <string>
#include <mutex>

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