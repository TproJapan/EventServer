#pragma once
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <map>
#include <thread>
#include "CSocketMap.h"

///////////////////////////////////////////////////////////////////////////////
// ���p�ϐ�
///////////////////////////////////////////////////////////////////////////////
int server_status = 0;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
//bool main_thread_flag = true;
HANDLE	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	32					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
CSocketMap* pSocketMap;