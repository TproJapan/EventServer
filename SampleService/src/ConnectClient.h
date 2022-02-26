#pragma once
#include <mutex>
///////////////////////////////////////////////////////////////////////////////
//�N���C�A���g�ڑ��N���X
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
private:
	SOCKET _socket;
public:
	bool _live;//�����Ǘ��t���O
	std::mutex	m_mutex;//�����Ǘ��t���O�p�r��
public:
	ConnectClient(SOCKET& tmpsocket);
	~ConnectClient();

	///////////////////////////////////////////////////////////////////////////////
	//�n���h������
	///////////////////////////////////////////////////////////////////////////////
public:
	void func();

private:
	// �N���C�A���g����̃f�[�^��t���̃n���h��
	bool recvHandler(HANDLE& hEvent);

	// �N���C�A���g�Ƃ̒ʐM�\�P�b�g�̐ؒf���m���̃n���h��
	bool closeHandler(HANDLE& hEvent);

	// �w�肳�ꂽ�C�x���g�n���h���ƃ\�P�b�g�N���[�Y�Amap����̍폜
	void deleteConnection(HANDLE& hEvent);
};