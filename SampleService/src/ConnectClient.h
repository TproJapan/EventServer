#pragma once
#include <mutex>
#ifdef __GNUC__
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
private:
	int  _dstSocket;
public:
	bool _live;//生存管理フラグ
	std::mutex	m_mutex;//生存管理フラグ用排他
public:
	ConnectClient(int dstSocket);
	~ConnectClient();
public:
	void func();
};
#else


class ConnectClient {
private:
	SOCKET _socket;
public:
	bool _live;//生存管理フラグ
	std::mutex	m_mutex;//生存管理フラグ用排他
public:
	ConnectClient(SOCKET& tmpsocket);
	~ConnectClient();

	///////////////////////////////////////////////////////////////////////////////
	//ハンドラ制御
	///////////////////////////////////////////////////////////////////////////////
public:
	void func();

private:
	// クライアントからのデータ受付時のハンドラ
	bool recvHandler(HANDLE& hEvent);

	// クライアントとの通信ソケットの切断検知時のハンドラ
	bool closeHandler(HANDLE& hEvent);

	// 指定されたイベントハンドルとソケットクローズ、mapからの削除
	void deleteConnection(HANDLE& hEvent);
};
#endif