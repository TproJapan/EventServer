#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include "CommonVariables.h"
#include "CommonFunc.h"
#include <boost/asio.hpp>
#include "CommonVariables.h"
#include "BoostLog.h"


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
	char log_buff[1024];

	HANDLE tmpEvent = WSACreateEvent();

	//socket�ƃC�x���g�ϐ����A�ǂ̊ϓ_�̃C�x���g�Ŕ��������邩��R�Â�
	int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		sprintf(log_buff, "WSAEventSelect error. (%ld)\n", WSAGetLastError());
		write_log(5,log_buff);
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
		sprintf(log_buff, "�������݂�҂��Ă��܂�.\n");
		write_log(2,log_buff);
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
			sprintf(log_buff, "WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			write_log(5,log_buff);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			printf("�^�C���A�E�g�����ł�!!!\n");
			sprintf(log_buff, "�^�C���A�E�g�����ł�!!!\n");
			write_log(2,log_buff);
			continue;
		}

		printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		sprintf(log_buff, "WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		write_log(2,log_buff);

		// �C�x���g�����m����HANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//�C�x���g����
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			sprintf(log_buff, "WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			write_log(5,log_buff);
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
	sprintf(log_buff, "Finished Thead\n");
	write_log(2,log_buff);
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g����̃f�[�^��t���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	char log_buff[1024];
	SOCKET sock = socketMap.getSocket(hEvent);
	printf("�N���C�A���g(%ld)����f�[�^����M\n", sock);
	sprintf(log_buff, "�N���C�A���g(%ld)����f�[�^����M\n", sock);
	write_log(2,log_buff);

	char buf[1024];
	int stSize = recv(sock, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		printf("recv error.\n");
		sprintf(log_buff, "recv error.\n");
		write_log(4,log_buff);
		printf("�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���\n", sock);
		sprintf(log_buff, "�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���\n", sock);
		write_log(4,log_buff);

		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("�ϊ��O:[%s] ==> ", buf);
	sprintf(log_buff, "�ϊ��O:[%s] ==> ", buf);
	write_log(2,log_buff);

	for (int i = 0; i < (int)stSize; i++) { // buf�̒��̏�������啶���ɕϊ�
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// �N���C�A���g�ɕԐM
	stSize = send(sock, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		printf("send error.\n");
		sprintf(log_buff, "send error.\n");
		write_log(4,log_buff);
		printf("�N���C�A���g�Ƃ̐ڑ����؂�܂���\n");
		sprintf(log_buff, "�N���C�A���g�Ƃ̐ڑ����؂�܂���\n");
		write_log(4,log_buff);

		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("�ϊ���:[%s] \n", buf);
	sprintf(log_buff, "�ϊ���:[%s] \n", buf);
	write_log(2,log_buff);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	char log_buff[1024];
	SOCKET sock = socketMap.getSocket(hEvent);

	printf("�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���\n", sock);
	sprintf(log_buff, "�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���\n", sock);
	write_log(4,log_buff);
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