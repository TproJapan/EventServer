#pragma once
#include "CSocketMap.h"

///////////////////////////////////////////////////////////////////////////////
// ���p�ϐ�
///////////////////////////////////////////////////////////////////////////////
#if defined __MAIN_SRC__
int server_status = 0;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
std::mutex	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	2					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
CSocketMap* pSocketMap;

#else
extern int server_status;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
extern std::mutex	server_status_Mutex;
extern HANDLE socketMap_Mutex;
extern const char* PIPE_NAME;
#define CLIENT_MAX	2					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
extern CSocketMap* pSocketMap;
#endif