#include <windows.h>
#include <iostream>

using namespace std;

void ErrorHandler(const char* s, DWORD err)
{
	cout << s << endl;
	cout << "エラー番号: " << err << endl;
	ExitProcess(err);
}

int main(int argc, char* argv[])
{
	SC_HANDLE	newService;
	SC_HANDLE	scm;

	if (argc != 4)
	{
		cout << "使用方法:\n";
		cout << "  install <servicename> <service_label> <executable>\n";
		return -1;
	}

	cout << "インストールを開始します...\n";

	// SCMへの接続を開く
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		ErrorHandler("OpenSCManagerでエラーが発生", GetLastError());
	}

	cout << "サービス名 : " << argv[1] << endl;
	cout << "サービスの表示名 : " << argv[2] << endl;
	cout << "実行ファイルのパス名 : " << argv[3] << endl;

	// 新しいサービスをインストールする
	newService = CreateService(scm,
		argv[1],
		argv[2],
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		argv[3],
		0, 0, 0, 0, 0);

	if (!newService)
	{
		ErrorHandler("CreateServiceでエラーが発生", GetLastError());
	}

	cout << "サービスはインストールされました\n";

	// クリーンアップ
	CloseServiceHandle(newService);
	CloseServiceHandle(scm);

	cout << "インストールを終了します...\n";
	return 0;
}




