#pragma once
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include "ConnectClient.h"
#include "CommonVariables.h"

int Tcpserver();
void StopTcpServer();//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)		
int cleanupConnectClientVec(connectclient_vector& vec);
