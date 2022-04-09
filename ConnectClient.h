#pragma once
#include <mutex>

#ifndef _WIN64
typedef int SOCKET;
#endif
///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	public:
		bool _live;//生存管理フラグ
		std::mutex	m_mutex;//生存管理フラグ用排他
		~ConnectClient();
		void func();
	private:
		int  _socket;
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

		// 指定されたイベントハンドルとソケットクローズ、mapからの削除
		void deleteConnection(HANDLE& hEvent);
#endif
};