#pragma once
//#include <stdio.h>
//#pragma comment(lib, "ws2_32.lib")
//#pragma warning(disable:4996)
//#include <WinSock2.h>
//#include <map>
//#include <thread>

//#include <windows.h>
#include "CSocketMap.h"

///////////////////////////////////////////////////////////////////////////////
// ���p�ϐ�
///////////////////////////////////////////////////////////////////////////////
#if defined __MAIN_SRC__
int server_status = 0;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
//bool main_thread_flag = true;
HANDLE	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	4					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
CSocketMap* pSocketMap;

#else
extern int server_status;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
//bool main_thread_flag = true;
extern HANDLE	server_status_Mutex;
extern HANDLE socketMap_Mutex;
extern const char* PIPE_NAME;
#define CLIENT_MAX	4					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
extern CSocketMap* pSocketMap;
#endif