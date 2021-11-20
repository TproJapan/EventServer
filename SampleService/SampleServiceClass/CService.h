#pragma once

#include <windows.h>
#include "CLog.h"

class CService
{
public:
	//CService(CLog& log); 	// �R���X�g���N�^
	CService(); 	// �R���X�g���N�^
	~CService();			// �f�X�g���N�^
	bool run();				// �T�[�r�X�̊J�n
	static void setLog(CLog& log) { logObj = log; }

private:
	bool registServiceMain();
	static VOID ServiceMain(DWORD argc, LPSTR* argv);
	//CLog& logObj;
	static CLog& logObj;
};

