#pragma once
#include <mutex>
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	public:
		bool _live;//生存管理フラグ
		std::mutex	m_mutex;//生存管理フラグ用排他
		~ConnectClient();
		void func();
	#ifndef _WIN64
	private:
		int  _dstSocket;
	public:
		ConnectClient(int dstSocket);
	#else
	private:
		SOCKET _socket;
	public:
		ConnectClient(SOCKET& tmpsocket);
	private:
		// クライアントからのデータ受付時のハンドラ
		bool recvHandler(HANDLE& hEvent);

		// クライアントとの通信ソケットの切断検知時のハンドラ
		bool closeHandler(HANDLE& hEvent);

		// 指定されたイベントハンドルとソケットクローズ、mapからの削除
		void deleteConnection(HANDLE& hEvent);
	#endif
};