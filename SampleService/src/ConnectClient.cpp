#ifdef __GNUC__
#include "ConnectClient.h"
#include "BoostLog.h"
#include "TcpCommon.h"
#include <thread>
#include <sys/socket.h>
///////////////////////////////////////////////////////////////////////////////
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(int dstSocket)
{
	_dstSocket = dstSocket;
	_live = true;
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
	write_log(2, "func started. _dstSocket=%d, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);

	while (1) {
		//�I���m�F
		if (GetServerStatus() == 1) {
			write_log(2, "Thread:%s���I�����܂�, %s %d %s\n", std::this_thread::get_id(), __FILENAME__, __LINE__, __func__);
			break;
		}

		///////////////////////////////////
		// �ʐM
		///////////////////////////////////
		write_log(2, "client(%d)�N���C�A���g�Ƃ̒ʐM���J�n���܂�, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);
		size_t stSize;
		char buf[1024];

		// �^�C���A�E�g�̐ݒ�
		struct timeval  tval;
		tval.tv_sec = SELECT_TIMER_SEC;	// time_t  �b
		tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  �}�C�N���b

		fd_set  readfds;//�r�b�g�t���O�Ǘ��ϐ�
		FD_ZERO(&readfds);//������

		FD_SET(_dstSocket, &readfds);

		int nRet = select(FD_SETSIZE,
			&readfds,
			NULL,
			NULL,
			&tval);

		if (nRet == -1) {
			if (errno == EINTR) {//�V�O�i�����荞�݂͏��O
				continue;
			}
			else {
				// select���ُ�I��
				write_log(4, "select error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				break;
			}
		}
		else if (nRet == 0) {
			write_log(2, "worker�X���b�h�Ń^�C���A�E�g����, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		stSize = recv(_dstSocket,
			buf,
			sizeof(buf),
			0);
		if (stSize <= 0) {
			write_log(4, "recv error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			write_log(4, "�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);
			close(_dstSocket);
			break;
		}

		write_log(2, "�ϊ��O:[%s] ==> , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
		for (int i = 0; i < stSize; i++) { // buf�̒��̏�������啶���ɕϊ�
			if (isalpha(buf[i])) {
				buf[i] = toupper(buf[i]);
			}
		}

		// �N���C�A���g�ɕԐM
		stSize = send(_dstSocket,
			buf,
			strlen(buf) + 1,
			0);

		if (stSize != strlen(buf) + 1) {
			write_log(4, "send error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
			write_log(4, "�N���C�A���g�Ƃ̐ڑ����؂�܂���, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			close(_dstSocket);
			break;
		}
		write_log(2, "�ϊ���:[%s] , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	}
	//while��������t���O�|��
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;
}
#else


#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include "TcpCommon.h"
#include <boost/asio.hpp>
#include "BoostLog.h"


///////////////////////////////////////////////////////////////////////////////
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(SOCKET& tmpsocket)
{
	_socket = tmpsocket;
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
		write_log(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		return;
	}

	//�C�x���g��z��ŊǗ�
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//�T�[�o�[�X�e�[�^�X�`�F�b�N
		if (checkServerStatus() == 1) break;

		//�����I���p��interruption_point�𒣂�
		boost::this_thread::interruption_point();

		write_log(2, "�������݂�҂��Ă��܂�.,%s %d %s\n", __FILENAME__, __LINE__, __func__);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//�C�x���g���d�҂�
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
			workerEventList,
			FALSE,
			worker_dwTimeout,
			FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			write_log(5, "WSAWaitForMultipleEvents wait error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			write_log(2, "�^�C���A�E�g�����ł�!!!, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		write_log(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", worker_nRet, __FILENAME__, __LINE__, __func__);

		// �C�x���g�����m����HANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//�C�x���g����
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�Ƀf�[�^����������
			recvHandler(workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̃N���[�Y�����m
			closeHandler(workerHandle);
		}
	}

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

	//while��������t���O�|��
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;

	write_log(2, "Finished Thead, %s %d %s\n", __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g����̃f�[�^��t���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::recvHandler(HANDLE& hEvent)
{
	write_log(2, "�N���C�A���g(%ld)����f�[�^����M, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		write_log(4, "recv error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		write_log(4, "�N���C�A���g(%ld)�Ƃ̐ڑ����؂�܂���, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	write_log(2, "�ϊ��O:[%s] ==> %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);

	for (int i = 0; i < (int)stSize; i++) { // buf�̒��̏�������啶���ɕϊ�
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// �N���C�A���g�ɕԐM
	stSize = send(_socket, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		write_log(4, "send error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		write_log(4, "�N���C�A���g�Ƃ̐ڑ����؂�܂���, %s %d %s\n", __FILENAME__, __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	write_log(2, "�ϊ���:[%s] %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(HANDLE& hEvent)
{
	write_log(4, "�N���C�A���g(%d)�Ƃ̐ڑ����؂�܂���, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
	deleteConnection(hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::deleteConnection(HANDLE& hEvent)
{
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
#endif