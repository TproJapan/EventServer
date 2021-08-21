#include <WinSock2.h>
#pragma warning(disable:4996)
#include "CommonFunc.h"
#include "ConnectClient.h"
#include "CommonVariables.h"
#include <boost/bind.hpp>
#include "BoostLog.h"

int checkServerStatus()
{
	std::lock_guard<std::mutex> lk(server_status_Mutex);
	int status = server_status;
	return status;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp)
{
	printf("クライアント接続要求を受け付けました\n");
	char log_buff[1024];
	sprintf(log_buff, "クライアント接続要求を受け付けました\n");
	write_log(2,log_buff);

	int	addrlen;
	struct sockaddr_in dstAddr;
	addrlen = sizeof(dstAddr);

	SOCKET sock = socketMap.getSocket(hEvent);
	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		printf("accept error. (%ld)\n", WSAGetLastError());
		sprintf(log_buff, "accept error. (%ld)\n", WSAGetLastError());
		write_log(5,log_buff);
		return true;
	}

	printf("[%s]から接続を受けました. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);
	sprintf(log_buff, "[%s]から接続を受けました. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);
	write_log(2,log_buff);

	int socket_size = socketMap.getCount();
	printf(" socketMap.getCount():%d\n", socket_size);
	sprintf(log_buff, " socketMap.getCount():%d\n", socket_size);
	write_log(2,log_buff);

	if (socket_size == CLIENT_MAX + 2) //2は名前付きパイプ, listenソケット
	{
		printf("同時接続可能クライアント数を超過\n");
		sprintf(log_buff, "同時接続可能クライアント数を超過\n");
		write_log(2,log_buff);
		closesocket(newSock);
		return false;
	}

	//プールスレッドにバインド
	ConnectClient* h = new ConnectClient(newSock, pSocketMap);
	tp.post(boost::bind(&ConnectClient::func, h));

	return true;
}