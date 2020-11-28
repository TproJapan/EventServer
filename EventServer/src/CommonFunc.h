#pragma once
//#include <stdio.h>
//#pragma comment(lib, "ws2_32.lib")
/*
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <map>
#include <thread>
#include "CSocketMap.h"
#include "thread_pool.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "CommonVariables.h"
#include "ConnectClient.h"
*/

#include "thread_pool.h"
#include "ConnectClient.h"
#include "CSocketMap.h"

//相互参照を解消する為のクラス前方宣言
/* class ConnectClient; */ // konishi
//class thread_pool;

//プロトタイプ宣言
extern int checkServerStatus();
extern bool acceptHandler(CSocketMap& socketMap, HANDLE& hEvent, thread_pool& tp);
extern int checkServerStatus();
