#ifndef __GNUC__
///////////////////////////////////////////////////////////////////////////////
// サービスのインストールプログラムサンプル
///////////////////////////////////////////////////////////////////////////////
//#include "stdafx.h"
#include <tchar.h>
#include <windows.h>
#include <iostream>
using namespace std;

///////////////////////////////////////////////////////////////////////////////
// プロトタイプ宣言
///////////////////////////////////////////////////////////////////////////////
void ErrorHandler(const char* s, DWORD err);

#define SERVICE_NAME  L"SampleService"
#define SERVICE_DIPLAY_NAME  L"SampleServ"
#define SERVICE_DETAIL  L"テスト用のサービスです"
//#define SERVICE_EXE_FILEPATH  L"C:\\Users\\zx9y-\\source\\repos\\SampleService\\x64\\Debug\\SampleService.exe"
#define SERVICE_EXE_FILEPATH  L"C:\\Users\\suguru.tateda\\VSProjects\\EventServer\\SampleService\\x64\\Debug\\SampleService.exe"
///////////////////////////////////////////////////////////////////////////////
// メイン処理
// install SampleService "SampleServ" C:\Users\zx9y-\source\repos\SampleService\Debug\SampleService.exe
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	SC_HANDLE	newService;
	SC_HANDLE	scm;

	cout << "インストールを開始します...\n";

	// SCMへの接続を開く
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		//ErrorHandler("OpenSCManagerでエラーが発生", GetLastError());
		std::printf("%ld:%s", GetLastError(), "OpenSCManagerでエラーが発生\n");
		return -1;
	}

	// 新しいサービスをインストールする
	newService = CreateService(scm,
							SERVICE_NAME,
							SERVICE_DIPLAY_NAME,
							SERVICE_ALL_ACCESS,
							SERVICE_WIN32_OWN_PROCESS,
							SERVICE_DEMAND_START,
							SERVICE_ERROR_NORMAL,
							SERVICE_EXE_FILEPATH,
							0, 0, 0, 0, 0);

	if (!newService)
	{
		//ErrorHandler("CreateServiceでエラーが発生", GetLastError());
		std::printf("%ld:%s", GetLastError(), "CreateServiceでエラーが発生\n");
		return -1;
	}

	cout << "サービスはインストールされました\n";

	// クリーンアップ
	CloseServiceHandle(newService);
	CloseServiceHandle(scm);

	cout << "インストールを終了します...\n";
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// エラーハンドラー
///////////////////////////////////////////////////////////////////////////////
void ErrorHandler(const char* s, DWORD err)
{
	//cout << s << endl;
	//cout << "エラー番号: " << err << endl;
	std::printf("%ld:%s", err, s);
	ExitProcess(err);
}
#endif


