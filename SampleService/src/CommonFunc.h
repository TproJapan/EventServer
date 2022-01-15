#pragma once
#include "thread_pool.h"
#include "ConnectClient.h"
#include "CSocketMap.h"

//プロトタイプ宣言
extern int checkServerStatus();
extern bool acceptHandler(SOCKET& sock, thread_pool& tp);
