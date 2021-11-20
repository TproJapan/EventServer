#pragma once
#include <map>
#include <mutex>
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////
// CSocketMap
///////////////////////////////////////////////////////////////////////////////
class CSocketMap
{
public:
	static CSocketMap* getInstance();

	//CSocketMap();								// コンストラクタ
	~CSocketMap();								// デストラクタ
	bool addSocket(SOCKET socket, HANDLE handle); // ソケットの登録
	SOCKET getSocket(HANDLE handle);			  // ソケットの取得
	bool deleteSocket(HANDLE handle);			  // ソケットの削除
	int getCount();								  // 登録済みソケット数の取得

private:
	static CSocketMap* pInstance;
	CSocketMap();								// コンストラクタ
	std::map<HANDLE, SOCKET> m_socketMap;		// ソケットmap
	std::mutex	m_mutex;						// 排他用mutex
	bool findSocket(HANDLE handle);			  // ソケット検索 
};

