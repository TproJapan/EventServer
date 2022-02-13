#include <WinSock2.h>
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/bind.hpp>
#include "BoostLog.h"

int checkServerStatus()
{
	std::lock_guard<std::mutex> lk(server_status_Mutex);
	int status = server_status;
	return status;
}