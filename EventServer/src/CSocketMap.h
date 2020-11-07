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

	//CSocketMap();								// �R���X�g���N�^
	~CSocketMap();								// �f�X�g���N�^
	bool addSocket(SOCKET socket, HANDLE handle); // �\�P�b�g�̓o�^
	SOCKET getSocket(HANDLE handle);			  // �\�P�b�g�̎擾
	bool deleteSocket(HANDLE handle);			  // �\�P�b�g�̍폜
	int getCount();								  // �o�^�ς݃\�P�b�g���̎擾

private:
	static CSocketMap* pInstance;
	CSocketMap();								// �R���X�g���N�^
	std::map<HANDLE, SOCKET> m_socketMap;		// �\�P�b�gmap
	std::mutex	m_mutex;						// �r���pmutex
	bool findSocket(HANDLE handle);			  // �\�P�b�g���� 
};

