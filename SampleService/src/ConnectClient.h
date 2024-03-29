﻿#pragma once
#include <mutex>

#ifndef _WIN64
typedef int SOCKET;
#endif
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
public:
	bool live;//生存管理フラグ
	std::mutex	mMutex;//生存管理フラグ用排他
	~ConnectClient();
	void func();
private:
	SOCKET  _socket;
public:
	ConnectClient(SOCKET dstSocket);
private:
#ifdef _WIN64
	// クライアントからのデータ受付時のハンドラ
	bool recvHandler(HANDLE& hEvent);
#else
	bool recvHandler();
#endif
#ifdef _WIN64
	// クライアントとの通信ソケットの切断検知時のハンドラ
	bool closeHandler(HANDLE& hEvent);

	// CloseHandleのラッパー
	bool closeAndInvalidateHandle(HANDLE& hEvent);

	// 指定されたイベントハンドルとソケットクローズ、mapからの削除
	void deleteConnection(HANDLE& hEvent);
#endif
	// closeまたはclosesocketのラッパー
	bool closeAndInvalidateSocket(SOCKET& socket);

};