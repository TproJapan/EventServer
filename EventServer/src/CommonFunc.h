#pragma once
#include "thread_pool.h"
#include "ConnectClient.h"
#include "CSocketMap.h"

//�v���g�^�C�v�錾
extern int checkServerStatus();
extern bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp);
extern int checkServerStatus();
