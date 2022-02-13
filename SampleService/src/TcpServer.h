#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "TcpCommon.h"
#include "ConnectClient.h"

typedef std::vector<ConnectClient*> connectclient_vector;
int Tcpserver();
void StopTcpServer();//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)		
int cleanupConnectClientVec(connectclient_vector& vec);
bool acceptHandler(SOCKET& sock, thread_pool& tp);

#if defined __MAIN_SRC__
HANDLE TcpServerMainEnd = INVALID_HANDLE_VALUE;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#else
extern HANDLE TcpServerMainEnd;// Tcpserver�̏I�����T�[�r�X�ɒm�点��ׂɎg�p����C�x���g
#endif
