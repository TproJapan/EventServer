#pragma once
#ifndef _WIN64
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>	
#include <string>
#include <string.h>
#include <mutex>
#else
#include "ThreadPool.h"
#include <vector>
#endif

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
int getServerStatus();

#ifndef _WIN64
#define PIPE_START "/tmp/fifo_start"//Start�Ɋ����񍐂��閼�O�t���p�C�v
#define PIPE_STOP "/tmp/fifo_stop"//Stop�Ɋ����񍐂��閼�O�t���p�C�v
#define REQUEST_PIPE "/tmp/request_pipe"
#define PID_SERVER "/tmp/myServer.pid"
#define PID_LOG "/tmp/myLog.pid"
#define PID_OBSERVER "/tmp/Observer.pid"
#define TMP_LOGFILE "/tmp/log"
#define QUE_NAME "/mq_stop_observer"
// #define PROJ_HOME "/home/tateda/EventServer/SampleService/src"
#define CLIENT_MAX	800 //�}�V�[�����\�[�X�Ɉˑ����鐔
#define SELECT_TIMER_SEC	30			// select�̃^�C�}�[(�b)
#define SELECT_TIMER_USEC	0			// select�̃^�C�}�[(�}�C�N���b)
int setServerStatus(int status);
#if defined __TCPCOMMON__
int serverStatus = 0; //�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
std::mutex	serverStatusMutex;
#else
extern int serverStatus;
extern std::mutex	serverStatusMutex;
#endif
#else
#define CLIENT_MAX	400					// �����ڑ��\�N���C�A���g��
#define TIMEOUT_MSEC	3000			// �^�C���A�E�g����(�~���b)
#if defined __MAIN_SRC__
int serverStatus = 0;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
std::mutex	serverStatusMutex;
HANDLE socketMapMutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#else
extern int serverStatus;//�T�[�o�[�X�e�[�^�X(0:�N��, 1:�V���b�g�_�E��)
extern std::mutex	serverStatusMutex;
extern HANDLE socketMapMutex;
extern const char* PIPE_NAME;
#endif
#endif