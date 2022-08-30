#ifndef _WIN64
#define __TCPCOMMON__
#endif
#include "TcpCommon.h"

#ifndef _WIN64
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
#include <syslog.h>
#include <string>
#else
#include <WinSock2.h>
#pragma warning(disable:4996)
#include "ConnectClient.h"
#include <boost/bind.hpp>
#include "BoostLog.h"
#endif

int getServerStatus() {
	std::lock_guard<std::mutex> lock(serverStatusMutex);
	return serverStatus;
}

#ifndef _WIN64
int setServerStatus(int status) {
	std::lock_guard<std::mutex> lock(serverStatusMutex);
	serverStatus = status;
	return serverStatus;
}
#endif