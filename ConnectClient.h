#pragma once
#include <mutex>

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