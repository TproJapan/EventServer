#pragma once
//#include "CSocketMap.h"
#include <mutex>
///////////////////////////////////////////////////////////////////////////////
//クライアント接続クラス
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
private:
	SOCKET _socket;
public:
	bool _live;//生存管理フラグ
	std::mutex	m_mutex;//生存管理フラグ用排他
public:
	//ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr);
	ConnectClient(SOCKET& tmpsocket);
	~ConnectClient();

	///////////////////////////////////////////////////////////////////////////////
	//ハンドラ制御
	///////////////////////////////////////////////////////////////////////////////
public:
	void func();

private:
	// クライアントからのデータ受付時のハンドラ
	//bool recvHandler(CSocketMap& socketMap, HANDLE& hEvent);
	bool recvHandler(HANDLE& hEvent);

	// クライアントとの通信ソケットの切断検知時のハンドラ
	//bool closeHandler(CSocketMap& socketMap, HANDLE& hEvent);
	bool closeHandler(HANDLE& hEvent);

	// 指定されたイベントハンドルとソケットクローズ、mapからの削除
	//void deleteConnection(CSocketMap& socketMap, HANDLE& hEvent);
	void deleteConnection(HANDLE& hEvent);
};