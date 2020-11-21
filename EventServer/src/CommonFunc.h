#pragma once
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <map>
#include <thread>
#include "CSocketMap.h"
#include "thread_pool.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "CommonVariables.h"
#include "ConnectClient.h"

//相互参照を解消する為のクラス前方宣言
class ConnectClient;

//プロトタイプ宣言
int checkServerStatus();
bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp);

int checkServerStatus() {
	WaitForSingleObject(server_status_Mutex, INFINITE); //mutex間は他のスレッドから変数を変更できない
	int status = server_status;
	ReleaseMutex(server_status_Mutex);
	return status;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp)
{
	printf("クライアント接続要求を受け付けました\n");
	int	addrlen;
	struct sockaddr_in dstAddr;
	addrlen = sizeof(dstAddr);

	SOCKET sock = socketMap.getSocket(hEvent);
	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		printf("accept error. (%ld)\n", WSAGetLastError());
		return true;
	}

	printf("[%s]から接続を受けました. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);

	WaitForSingleObject(socketMap_Mutex, INFINITE);
	int socket_size = socketMap.getCount();
	if (socket_size == CLIENT_MAX) {
		printf("同時接続可能クライアント数を超過\n");
		closesocket(newSock);
		return false;
	}

	//プールスレッドにバインド
	ConnectClient* h = new ConnectClient(newSock, pSocketMap);
	tp.post(boost::bind(&ConnectClient::func, h));

	return true;
}