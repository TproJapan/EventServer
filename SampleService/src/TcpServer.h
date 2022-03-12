#pragma once
#ifdef __GNUC__

#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <iostream>
#include <sys/wait.h>// wait
#include <err.h>	// err
#include <stdlib.h>	// exit
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <pthread.h>
#include "BoostLog.h"
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;

class TcpServer {
public:
	int nPortNo;
	boost::asio::io_service io_service;
	thread_pool tp;
	struct sockaddr_in srcAddr;
	int srcSocket;
public:
	TcpServer(int portNo);
	~TcpServer();
	int Func();
	// �\�P�b�g�ؒf�ς݂�connect_client�̉������
	int cleanupConnectClientVec(connectclient_vector& vec);
	static void sigalrm_handler(int signo, thread_pool _tp);
	static void sigusr2_handler(int signo);
};
#else

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;

class TcpServer {
public:
	int isInisialized;
	//int nRet;
	boost::asio::io_service io_service;
	thread_pool tp;

	char pipeName[80];	// �p�C�v��
	HANDLE hPipe;// ���O�t���p�C�v
	OVERLAPPED overlappedConnect;// OVARLAPPED�\����
	HANDLE eventConnect;

	//BOOL bRet;
	int nPortNo;
	WSADATA	WsaData;
	WORD wVersionRequested;
	SOCKET srcSocket;// �\�P�b�g(listen�p)
	HANDLE hEvent;

	struct sockaddr_in srcAddr;
public:
	TcpServer();
	~TcpServer();
	int Func();
	int Run();
	int Terminate();
	void StopTcpServer();
	int cleanupConnectClientVec(connectclient_vector& vec);
	bool acceptHandler(SOCKET& sock, thread_pool& tp);
};

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#else
extern HANDLE TcpServerMainEnd;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#endif

#endif