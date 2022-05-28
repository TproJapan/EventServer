#pragma once

#include <windows.h>
#include "CLog.h"

class CService
{
public:
	//CService(CLog& log); 	// コンストラクタ
	CService(); 	// コンストラクタ
	~CService();			// デストラクタ
	bool run();				// サービスの開始
	static void setLog(CLog& log) { logObj = log; }

private:
	bool registServiceMain();
	static VOID ServiceMain(DWORD argc, LPSTR* argv);
	//CLog& logObj;
	static CLog& logObj;
};

