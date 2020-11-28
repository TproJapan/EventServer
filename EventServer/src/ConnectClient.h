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
//�N���C�A���g�ڑ��N���X
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
	CSocketMap* _pSocketMap;
public:
	ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr);
	~ConnectClient();

	///////////////////////////////////////////////////////////////////////////////
	//�n���h������
	///////////////////////////////////////////////////////////////////////////////
public:
	void func();

private:
	// �N���C�A���g����̃f�[�^��t���̃n���h��
	bool recvHandler(CSocketMap& socketMap, HANDLE& hEvent);

	// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
	bool closeHandler(CSocketMap& socketMap, HANDLE& hEvent);

	// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y�Amap����̍폜
	void deleteConnection(CSocketMap& socketMap, HANDLE& hEvent);

//private:
	//int checkServerStatus();
};