#pragma once
#include <string>
#include <stdio.h>

class CLog
{
public:
	CLog();							// �R���X�g���N�^
	~CLog();						// �f�X�g���N�^
	bool open(const char* name);	// ���O�t�@�C���̃I�[�v��
	bool log(const char* fmt, ...);	// ���O�o��
	bool close();					// ���O�t�@�C���̃N���[�Y

private:
	void createTimeStamp(std::string& strTime);
									// �^�C���X�^���v�̍쐬
	FILE* _logFp;					// ���O�p�t�@�C���|�C���^			
};

