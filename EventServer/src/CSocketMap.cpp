///////////////////////////////////////////////////////////////////////////////
// �\�P�b�g�}�b�v�Ǘ��N���X
///////////////////////////////////////////////////////////////////////////////
#include "CSocketMap.h"
using namespace std;

CSocketMap* CSocketMap::pInstance = NULL;


///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �C���X�^���X�̎擾
// [����]
//  �Ȃ�
// [�߂�l]
//  �C���X�^���X�̃A�h���X
///////////////////////////////////////////////////////////////////////////////
CSocketMap* CSocketMap::getInstance()
{
	if (pInstance == NULL) {
		pInstance = new CSocketMap();
	}
	return pInstance;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �R���X�g���N�^
// [����]
//  �Ȃ�
// [�߂�l]
//   �Ȃ�
///////////////////////////////////////////////////////////////////////////////
CSocketMap::CSocketMap()
{
	return;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �f�X�g���N�^
// [����]
//  �Ȃ�
// [�߂�l]
//   �Ȃ�
///////////////////////////////////////////////////////////////////////////////
CSocketMap::~CSocketMap()

{
	lock_guard<mutex> lk(m_mutex);
	for (std::map<HANDLE, SOCKET>::iterator ite = m_socketMap.begin(); ite != m_socketMap.end(); ++ite)
	{
		CloseHandle(ite->first);
		closesocket(ite->second);
	}
	m_socketMap.clear();
	return;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �\�P�b�g�̓o�^
// [����]
//  socket : �ڑ��ς݃\�P�b�g
//  handle : �\�P�b�g�Ɗ֘A�t���ς݂�HANDLE
// [�߂�l]
//   true : ����I��
//   false: �ُ�I��
///////////////////////////////////////////////////////////////////////////////
bool CSocketMap::addSocket(SOCKET socket, HANDLE handle)
{
	lock_guard<mutex> lk(m_mutex);
	if (findSocket(handle)) {
		// �o�^�ς݂̃\�P�b�g�Ȃ̂ŃG���[
		return false;
	}
	m_socketMap[handle] = socket;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �\�P�b�g�̎擾
// [����]
//  handle : �\�P�b�g�Ɗ֘A�t���ς݂�HANDLE
// [�߂�l]
//   �\�P�b�g : ����I��
//   INVALID_SOCKET: �ُ�I��
///////////////////////////////////////////////////////////////////////////////
SOCKET CSocketMap::getSocket(HANDLE handle)
{
	lock_guard<mutex> lk(m_mutex);
	if (!findSocket(handle)) {
		// ���o�^�̃\�P�b�g�Ȃ̂ŃG���[
		return INVALID_SOCKET;
	}
	SOCKET socket = m_socketMap[handle];
	return socket;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �\�P�b�g�̍폜
// [����]
//  handle : �\�P�b�g�Ɗ֘A�t���ς݂�HANDLE
// [�߂�l]
//   true : ����I��
//   false: �ُ�I��
///////////////////////////////////////////////////////////////////////////////
bool CSocketMap::deleteSocket(HANDLE handle)
{
	lock_guard<mutex> lk(m_mutex);
	if (!findSocket(handle)) {
		// ���o�^�̃\�P�b�g�Ȃ̂ŃG���[
		return false;
	}
	SOCKET socket = m_socketMap[handle];
	if(socket != NULL) closesocket(socket);//2020.11.28�ǉ�
	CloseHandle(handle);
	m_socketMap.erase(handle);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �\�P�b�g�̌���
// [����]
//  handle : �\�P�b�g�Ɗ֘A�t���ς݂�HANDLE
// [�߂�l]
//   true : ����I��
//   false: �ُ�I��
///////////////////////////////////////////////////////////////////////////////
bool CSocketMap::findSocket(HANDLE handle)
{
	map<HANDLE, SOCKET>::iterator ite;
	ite = m_socketMap.find(handle);
	if (ite == m_socketMap.end()) {
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]
//  �o�^�ς݃\�P�b�g���̎擾
// [����]
//  �Ȃ�
// [�߂�l]
//   �o�^�ς݃\�P�b�g���̎擾
///////////////////////////////////////////////////////////////////////////////
int CSocketMap::getCount()
{
	lock_guard<mutex> lk(m_mutex);
	int count = (int)m_socketMap.size();
	return count;
}
