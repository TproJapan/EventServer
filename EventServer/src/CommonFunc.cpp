#include <WinSock2.h>
#pragma warning(disable:4996)
#include "CommonFunc.h"
#include "ConnectClient.h"
#include "CommonVariables.h"
#include <boost/bind.hpp>

int checkServerStatus()
{
	std::lock_guard<std::mutex> lk(server_status_Mutex);
	int status = server_status;
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
	
	int socket_size = socketMap.getCount();
	printf(" socketMap.getCount():%d\n", socket_size);
	if (socket_size == CLIENT_MAX + 2) //2�͖��O�t���p�C�v, listen�\�P�b�g
	{
		printf("�����ڑ��\�N���C�A���g���𒴉�\n");
		closesocket(newSock);
		return false;
	}

	//�v�[���X���b�h�Ƀo�C���h
	ConnectClient* h = new ConnectClient(newSock, pSocketMap);
	tp.post(boost::bind(&ConnectClient::func, h));

	return true;
}