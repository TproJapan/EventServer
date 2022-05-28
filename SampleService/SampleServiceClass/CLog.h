#pragma once
#include <string>
#include <stdio.h>

class CLog
{
public:
	CLog();							// コンストラクタ
	~CLog();						// デストラクタ
	bool open(const char* name);	// ログファイルのオープン
	bool log(const char* fmt, ...);	// ログ出力
	bool close();					// ログファイルのクローズ

private:
	void createTimeStamp(std::string& strTime);
									// タイムスタンプの作成
	FILE* _logFp;					// ログ用ファイルポインタ			
};

