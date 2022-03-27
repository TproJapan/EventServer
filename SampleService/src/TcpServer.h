#pragma once
#ifdef _WIN64
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#endif

#include <vector>
#include "TcpCommon.h"
#include "ConnectClient.h"
#ifdef _WIN64
// Windows��p�w�b�_�[

#else
// Unix��p�w�b�_�[

#endif

typedef std::vector<ConnectClient*> connectclient_vector;


class TcpServer {
public:
#ifdef _WIN64
	int isInisialized;
	HANDLE hPipe;// ���O�t���p�C�v
	OVERLAPPED overlappedConnect;// OVARLAPPED�\����
	HANDLE eventConnect;
	WORD wVersionRequested;
	SOCKET srcSocket;// �\�P�b�g(listen�p)
	HANDLE hEvent;
#else
	int srcSocket;
#endif
	boost::asio::io_service io_service;
	thread_pool tp;
	int nPortNo;
	struct sockaddr_in srcAddr;

public:
	TcpServer(int portNo);
	~TcpServer();
	int Func();
	int Terminate();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
#ifdef _WIN64
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
#else
	static void sigalrm_handler(int signo, thread_pool _tp);
	static void sigusr2_handler(int signo);
#endif
};

#ifdef _WIN64
#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#else
extern HANDLE TcpServerMainEnd;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#endif

#endif