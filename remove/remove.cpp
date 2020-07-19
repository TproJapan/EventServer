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
	SC_HANDLE	service;
	SC_HANDLE	scm;
	BOOL		success;
	SERVICE_STATUS	status;

	if (argc != 2)
	{
		cout << "使用方法:\n";
		cout << "  remove <servicename>\n";
		return -1;
	}

	cout << "削除を開始します...\n";

	// SCMへの接続を開く
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		ErrorHandler("OpenSCManagerでエラーが発生", GetLastError());
	}

	cout << "サービス名 : " << argv[1] << endl;

	// サービスのハンドルを取得する
	service = OpenService(scm, argv[1], SERVICE_ALL_ACCESS | DELETE);

	if (!service)
	{
		ErrorHandler("OpenServiceでエラーが発生", GetLastError());
	}


	// 必要ならサービスを停止する
	success = QueryServiceStatus(service, &status);
	if (!success)
	{
		ErrorHandler("QueryServiceStatusでエラーが発生", GetLastError());
	}

	if (status.dwCurrentState != SERVICE_STOPPED)
	{
		cout << "サービスを停止します...\n";

		success = ControlService(service, SERVICE_CONTROL_STOP, &status);
		if (!success)
		{
			ErrorHandler("ControlServiceでエラーが発生", GetLastError());
		}
		Sleep(5000);
	}

	// サービスを削除する
	success = DeleteService(service);
	if (!success)
	{
		ErrorHandler("DeleteServiceでエラーが発生", GetLastError());
	}

	cout << "サービスを削除しました\n";

	// クリーンアップ
	CloseServiceHandle(service);
	CloseServiceHandle(scm);

	cout << "削除を終了します...\n";
	return 0;
}




