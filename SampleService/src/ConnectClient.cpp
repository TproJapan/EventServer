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
ConnectClient::ConnectClient(SOCKET& tmpsocket)
{
	_socket = tmpsocket;
	//_pSocketMap = tmpSocketMapPtr;
	_live = true;
}

///////////////////////////////////////////////////////////////////////////////
// �f�X�g���N�^
///////////////////////////////////////////////////////////////////////////////
ConnectClient::~ConnectClient()
{
	if (_socket != INVALID_SOCKET) closesocket(_socket);
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
		//printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		write_log(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
		return;
	}

	//_pSocketMap->addSocket(_socket, tmpEvent);

	//�C�x���g��z��ŊǗ�
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//�T�[�o�[�X�e�[�^�X�`�F�b�N
		if (checkServerStatus() == 1) break;

		//�����I���p��interruption_point�𒣂�
		boost::this_thread::interruption_point();

		//printf("�������݂�҂��Ă��܂�.\n");
		write_log(2, "�������݂�҂��Ă��܂�.,%s %d %s\n", __FILENAME__ , __LINE__, __func__);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//�C�x���g���d�҂�
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
                                                   workerEventList,
                                                   FALSE,
			                                       worker_dwTimeout,
			                                       FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			//printf("WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			write_log(5, "WSAWaitForMultipleEvents wait error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			//printf("�^�C���A�E�g�����ł�!!!\n");
			write_log(2, "�^�C���A�E�g�����ł�!!!, %s %d %s\n", __FILENAME__ , __LINE__, __func__);
			continue;
		}

		//printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		write_log(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", worker_nRet, __FILENAME__ , __LINE__, __func__);

		// �C�x���g�����m����HANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//�C�x���g����
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			//printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�Ƀf�[�^����������
			//recvHandler(*pSocketMap, workerHandle);
			recvHandler(workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̃N���[�Y�����m
			//closeHandler(*pSocketMap, workerHandle);
			closeHandler(workerHandle);
		}
	}

	//deleteConnection(*pSocketMap, tmpEvent);
	//closesocket(_socket); // konishi
	//CloseHandle(tmpEvent);// konishi
#if 1
	if (_socket != INVALID_SOCKET) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (tmpEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(tmpEvent);
		tmpEvent = INVALID_HANDLE_VALUE;
	}
#endif

	//delete this;

	//while��������t���O�|��
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;

	//printf("Finished Thead\n");
	write_log(2, "Finished Thead, %s %d %s\n", __FILENAME__ , __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g����̃f�[�^��t���̃n���h��
///////////////////////////////////////////////////////////////////////////////
//bool ConnectClient::recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
bool ConnectClient::recvHandler(HANDLE& hEvent)
{
	//SOCKET sock = socketMap.getSocket(hEvent);
	//printf("�N���C�A���g(%ld)����f�[�^����M\n", sock);
	write_log(2, "�N���C�A���g(%ld)����f�[�^����M, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);

	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		//printf("recv error.\n");
		write_log(4, "recv error., %s %d %s\n", __FILENAME__ , __LINE__, __func__);
		//printf("�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���\n", sock);
		write_log(4, "�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	//printf("�ϊ��O:[%s] ==> ", buf);
	write_log(2, "�ϊ��O:[%s] ==> %s %d %s\n", buf, __FILENAME__ , __LINE__, __func__);

	for (int i = 0; i < (int)stSize; i++) { // buf�̒��̏�������啶���ɕϊ�
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// �N���C�A���g�ɕԐM
	stSize = send(_socket, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		//printf("send error.\n");
		write_log(4, "send error., %s %d %s\n", __FILENAME__ , __LINE__, __func__);
		//printf("�N���C�A���g�Ƃ̐ڑ����؂�܂���\n");
		write_log(4, "�N���C�A���g�Ƃ̐ڑ����؂�܂���, %s %d %s\n", __FILENAME__ , __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	//printf("�ϊ���:[%s] \n", buf);
	write_log(2, "�ϊ���:[%s] %s %d %s\n", buf, __FILENAME__ , __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
///////////////////////////////////////////////////////////////////////////////
//bool ConnectClient::closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
bool ConnectClient::closeHandler(HANDLE& hEvent)
{
	//SOCKET sock = socketMap.getSocket(hEvent);

	//printf("�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���\n", sock);
	write_log(4, "�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);
	deleteConnection(hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y
///////////////////////////////////////////////////////////////////////////////
//void ConnectClient::deleteConnection(CSocketMap& socketMap, HANDLE& hEvent)
void ConnectClient::deleteConnection(HANDLE& hEvent)
{
	//socketMap.deleteSocket(hEvent);
	//closesocket(_socket); // konishi
	//CloseHandle(hEvent);  // konishi

	if (_socket != INVALID_SOCKET) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(hEvent);
		hEvent = INVALID_HANDLE_VALUE;
	}

	return;
}