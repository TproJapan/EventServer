#pragma once
#include <mutex>

#ifndef _WIN64
typedef int SOCKET;
#endif
///////////////////////////////////////////////////////////////////////////////
//��������A�N���X�ł̊֐��I�u�W�F�N�g
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
public:
	bool _live;//�����Ǘ��t���O
	std::mutex	m_mutex;//�����Ǘ��t���O�p�r��
	~ConnectClient();
	void func();
private:
	int  _socket;
public:
	ConnectClient(SOCKET dstSocket);
#ifdef _WIN64
private:
	// �N���C�A���g����̃f�[�^��t���̃n���h��
	bool recvHandler(HANDLE& hEvent);

	// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
	bool closeHandler(HANDLE& hEvent);

	// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y�Amap����̍폜
	void deleteConnection(HANDLE& hEvent);
#endif
};