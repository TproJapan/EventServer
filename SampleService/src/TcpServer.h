#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;
//int Tcpserver();
class TcpServer {
public:
	int nRet;
	boost::asio::io_service io_service;
	thread_pool* tp_ptr;

	char pipeName[80];	// �p�C�v��
	HANDLE hPipe;// ���O�t���p�C�v
public:
	TcpServer();
	~TcpServer();
	int Func();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
};

//void StopTcpServer();//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)		
//int cleanupConnectClientVec(connectclient_vector& vec);
//bool acceptHandler(SOCKET& sock, thread_pool& tp);

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#else
extern HANDLE TcpServerMainEnd;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#endif
