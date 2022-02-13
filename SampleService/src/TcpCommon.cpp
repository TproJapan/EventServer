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

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool acceptHandler(SOCKET& sock, thread_pool& tp)
{
	//printf("クライアント接続要求を受け付けました\n");
	write_log(2, "クライアント接続要求を受け付けました %s %d %s\n", __FILENAME__ , __LINE__, __func__);

	int	addrlen;
	struct sockaddr_in dstAddr;
	addrlen = sizeof(dstAddr);

	//SOCKET sock = socketMap.getSocket(hEvent);
	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		//printf("accept error. (%ld)\n", WSAGetLastError());
		write_log(5, "accept error. (%ld) %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
		return true;
	}

	//printf("[%s]から接続を受けました. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);
	write_log(2, "[%s]から接続を受けました. newSock=%d, %s %d %s\n", inet_ntoa(dstAddr.sin_addr), newSock, __FILENAME__ , __LINE__, __func__);

	//int socket_size = socketMap.getCount();
	//printf("socketMap.getCount():%d\n", socket_size);
	//write_log(2, "socketMap.getCount():%d\n", socket_size);

	/*
	if (socket_size == CLIENT_MAX + 2) //2は名前付きパイプ, listenソケット
	{
		//printf("同時接続可能クライアント数を超過\n");
		write_log(2, "同時接続可能クライアント数を超過\n");
		closesocket(newSock);
		return false;
	}
	*/

	//プールスレッドにバインド
	//ConnectClient* h = new ConnectClient(newSock, pSocketMap);
	ConnectClient* h = new ConnectClient(newSock);
	tp.post(boost::bind(&ConnectClient::func, h));
	connectclient_vec.push_back(h);	//vectorに追加

	return true;
}