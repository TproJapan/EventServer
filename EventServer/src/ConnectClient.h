#pragma once
//#include <WinSock2.h>
#include "CSocketMap.h"

//#include <stdio.h>
//#pragma comment(lib, "ws2_32.lib")
//#pragma warning(disable:4996)
/*
#include <map>
#include <thread>
#include "thread_pool.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "CommonVariables.h"
#include "CommonFunc.h"
*/
///////////////////////////////////////////////////////////////////////////////
//クライアント接続クラス
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
	CSocketMap* _pSocketMap;
public:
	ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr);
	~ConnectClient();

	///////////////////////////////////////////////////////////////////////////////
	//ハンドラ制御
	///////////////////////////////////////////////////////////////////////////////
public:
	void func();

private:
	// クライアントからのデータ受付時のハンドラ
	bool recvHandler(CSocketMap& socketMap, HANDLE& hEvent);

	// クライアントとの通信ソケットの切断検知時のハンドラ
	bool closeHandler(CSocketMap& socketMap, HANDLE& hEvent);

	// 指定されたイベントハンドルとソケットクローズ、mapからの削除
	void deleteConnection(CSocketMap& socketMap, HANDLE& hEvent);

//private:
	//int checkServerStatus();
};