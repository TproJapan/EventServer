#include "CLog.h"
///////////////////////////////////////////////////////////////////////////////
// ログファイル管理クラス
///////////////////////////////////////////////////////////////////////////////
#pragma warning(disable:4996)
#include "CLog.h"
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <stdarg.h>

#else
#include <sys/time.h>
#endif

using namespace std;

#ifdef _WIN32
int vasprintf(char** strp, const char* fmt, va_list ap);
#endif

///////////////////////////////////////////////////////////////////////////////
// コンストラクタ
///////////////////////////////////////////////////////////////////////////////
CLog::CLog()
{
	_logFp = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// デストラクタ
///////////////////////////////////////////////////////////////////////////////
CLog::~CLog()
{
	close();
}

///////////////////////////////////////////////////////////////////////////////
// [機能]ログファイルのオープン(新規に作成する)
// [引数] name:ログファイル名
// [戻り値] true:正常終了, false:異常終了
///////////////////////////////////////////////////////////////////////////////
bool CLog::open(const char* name)
{
	if (name == NULL) {
		return false;
	}

	FILE* fp = fopen(name, "w");
	if (fp == NULL) {
		return false;
	}

	_logFp = fp;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// [機能]ログファイルをクローズする。
// [引数] なし
// [戻り値] true:正常終了, false:異常終了
// クローズ
///////////////////////////////////////////////////////////////////////////////
bool CLog::close()
{
	if (_logFp == NULL) {
		return false;
	}

	fclose(_logFp);
	_logFp = NULL;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// ログ出力
///////////////////////////////////////////////////////////////////////////////
bool CLog::log(const char* fmt, ...)
{

	if (_logFp == NULL) {
		return false;
	}

	va_list ap;
	char* allocBuf;
	va_start(ap, fmt);
	int nSize = vasprintf(&allocBuf, fmt, ap);
	va_end(ap);

	if (nSize < 0) {
		return false;
	}

	string strTimeStamp;
	createTimeStamp(strTimeStamp);

	fprintf(_logFp,
			"[%s] %s\n",
			strTimeStamp.c_str(),
			allocBuf);
	free(allocBuf);
	fflush(_logFp);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// タイムスタンプの作成
///////////////////////////////////////////////////////////////////////////////
void CLog::createTimeStamp(std::string& strTime)
{
#ifdef _WIN32
	SYSTEMTIME st;
	GetSystemTime(&st);
	char szTime[25] = { 0 };
	// wHourを９時間足して、日本時間にする
	::sprintf(szTime,
				"%04d/%02d/%02d %02d:%02d:%02d.%03d",
				st.wYear, st.wMonth, st.wDay,
				st.wHour + 9, st.wMinute, st.wSecond, st.wMilliseconds);
	strTime = szTime;
#else
	struct timespec  now;
	clock_gettime(CLOCK_REALTIME, &now);
	struct tm local;
	localtime_r(&now.tv_sec, &local);

	char timebuff[32];
	sprintf(timebuff,
		"%04d/%02d/%02d %02d:%02d:%02d.%03ld",
		local.tm_year + 1900,
		local.tm_mon + 1,
		local.tm_mday,
		local.tm_hour,
		local.tm_min,
		local.tm_sec,
		now.tv_nsec / 1000000);
	strTime = timebuff;
#endif
	return;
}

#ifdef _WIN32
///////////////////////////////////////////////////////////////////////////////
// w2indows用のvasprintf
///////////////////////////////////////////////////////////////////////////////
int vasprintf(char** strp, const char* fmt, va_list ap)
{
	char* buf = (char*)malloc(_vscprintf(fmt, ap) + 1);
	if (buf == NULL) {
		if (strp != NULL) {
			*strp = NULL;
		}
		return -1;
	}

	*strp = buf;
	return vsprintf(buf, fmt, ap);
}
#endif
