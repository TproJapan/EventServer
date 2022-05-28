#include "CLog.h"
///////////////////////////////////////////////////////////////////////////////
// ���O�t�@�C���Ǘ��N���X
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
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////
CLog::CLog()
{
	_logFp = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// �f�X�g���N�^
///////////////////////////////////////////////////////////////////////////////
CLog::~CLog()
{
	close();
}

///////////////////////////////////////////////////////////////////////////////
// [�@�\]���O�t�@�C���̃I�[�v��(�V�K�ɍ쐬����)
// [����] name:���O�t�@�C����
// [�߂�l] true:����I��, false:�ُ�I��
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
// [�@�\]���O�t�@�C�����N���[�Y����B
// [����] �Ȃ�
// [�߂�l] true:����I��, false:�ُ�I��
// �N���[�Y
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
// ���O�o��
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
// �^�C���X�^���v�̍쐬
///////////////////////////////////////////////////////////////////////////////
void CLog::createTimeStamp(std::string& strTime)
{
#ifdef _WIN32
	SYSTEMTIME st;
	GetSystemTime(&st);
	char szTime[25] = { 0 };
	// wHour���X���ԑ����āA���{���Ԃɂ���
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
// w2indows�p��vasprintf
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
