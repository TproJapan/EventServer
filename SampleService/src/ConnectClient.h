#pragma once
#include "CSocketMap.h"
///////////////////////////////////////////////////////////////////////////////
//クライアント接続クラス
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
	CSocketMap* _pSocketMap;
public:
	bool _live;//生存管理フラグ
	std::mutex	m_mutex;//生存管理フラグ用排他
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
};