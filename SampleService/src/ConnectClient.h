#pragma once
#include "CSocketMap.h"
///////////////////////////////////////////////////////////////////////////////
//�N���C�A���g�ڑ��N���X
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
	CSocketMap* _pSocketMap;
public:
	bool _live;//�����Ǘ��t���O
	std::mutex	m_mutex;//�����Ǘ��t���O�p�r��
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
};