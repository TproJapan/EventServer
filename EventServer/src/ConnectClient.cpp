#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include "CommonVariables.h"
#include "CommonFunc.h"
#include <boost/asio.hpp>


///////////////////////////////////////////////////////////////////////////////
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr)
{
	_socket = tmpsocket;
	_pSocketMap = tmpSocketMapPtr;
}

///////////////////////////////////////////////////////////////////////////////
// �f�X�g���N�^
///////////////////////////////////////////////////////////////////////////////
ConnectClient::~ConnectClient()
{

}

///////////////////////////////////////////////////////////////////////////////
//�n���h������
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::func()
{
	HANDLE tmpEvent = WSACreateEvent();

	//socket�ƃC�x���g�ϐ����A�ǂ̊ϓ_�̃C�x���g�Ŕ��������邩��R�Â�
	int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		return;
	}

	_pSocketMap->addSocket(_socket, tmpEvent);

	//�C�x���g��z��ŊǗ�
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//�T�[�o�[�X�e�[�^�X�`�F�b�N
		if (checkServerStatus() == 1) break;

		//�����I���p��interruption_point�𒣂�
		boost::this_thread::interruption_point();

		printf("�������݂�҂��Ă��܂�.\n");
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//�C�x���g���d�҂�
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
                                                   workerEventList,
                                                   FALSE,
			                                       worker_dwTimeout,
			                                       FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			printf("�^�C���A�E�g�����ł�!!!\n");
			continue;
		}

		printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);

		// �C�x���g�����m����HANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//�C�x���g����
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�Ƀf�[�^����������
			recvHandler(*pSocketMap, workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̃N���[�Y�����m
			closeHandler(*pSocketMap, workerHandle);
		}
	}

	deleteConnection(*pSocketMap, tmpEvent);
	delete this;
	printf("Finished Thead\n");
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g����̃f�[�^��t���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	SOCKET sock = socketMap.getSocket(hEvent);
	printf("�N���C�A���g(%ld)����f�[�^����M\n", sock);

	char buf[1024];
	int stSize = recv(sock, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		printf("recv error.\n");
		printf("�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���\n", sock);
		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("�ϊ��O:[%s] ==> ", buf);
	for (int i = 0; i < (int)stSize; i++) { // buf�̒��̏�������啶���ɕϊ�
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// �N���C�A���g�ɕԐM
	stSize = send(sock, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		printf("send error.\n");
		printf("�N���C�A���g�Ƃ̐ڑ����؂�܂���\n");
		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("�ϊ���:[%s] \n", buf);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	SOCKET sock = socketMap.getSocket(hEvent);

	printf("�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���\n", sock);
	deleteConnection(socketMap, hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y�Amap����̍폜
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::deleteConnection(CSocketMap& socketMap, HANDLE& hEvent)
{
	socketMap.deleteSocket(hEvent);
	return;
}