//#pragma once
/*
#include <stdio.h>
//#pragma comment(lib, "ws2_32.lib")
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
*/

#include <WinSock2.h>
//#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "CommonFunc.h"
#include "ConnectClient.h"
#include "CommonVariables.h"
#include <boost/bind.hpp>


int checkServerStatus()
{
	WaitForSingleObject(server_status_Mutex, INFINITE); //mutex�Ԃ͑��̃X���b�h����ϐ���ύX�ł��Ȃ�
	int status = server_status;
	ReleaseMutex(server_status_Mutex);
	return status;
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g����V�K�ڑ���t���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp)
{
	printf("�N���C�A���g�ڑ��v�����󂯕t���܂���\n");
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

	printf("[%s]����ڑ����󂯂܂���. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);
	
	//�ȉ��R�����g�A�E�g�����ꍇ�A3�߃N���C�A���g�ڑ��v�����Ƀv�[���ɋ󂫂��Ȃ����C���X���b�h���ł܂�
	/*
	//WaitForSingleObject(socketMap_Mutex, INFINITE);
	int socket_size = socketMap.getCount();
	if (socket_size == CLIENT_MAX) {
		printf("�����ڑ��\�N���C�A���g���𒴉�\n");
		closesocket(newSock);
		return false;
	}
	*/
	//�v�[���X���b�h�Ƀo�C���h
	ConnectClient* h = new ConnectClient(newSock, pSocketMap);
	tp.post(boost::bind(&ConnectClient::func, h));

	return true;
}