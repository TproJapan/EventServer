#include <windows.h>
#include <iostream>

using namespace std;

void ErrorHandler(const char* s, DWORD err)
{
	cout << s << endl;
	cout << "�G���[�ԍ�: " << err << endl;
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
		cout << "�g�p���@:\n";
		cout << "  remove <servicename>\n";
		return -1;
	}

	cout << "�폜���J�n���܂�...\n";

	// SCM�ւ̐ڑ����J��
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		ErrorHandler("OpenSCManager�ŃG���[������", GetLastError());
	}

	cout << "�T�[�r�X�� : " << argv[1] << endl;

	// �T�[�r�X�̃n���h�����擾����
	service = OpenService(scm, argv[1], SERVICE_ALL_ACCESS | DELETE);

	if (!service)
	{
		ErrorHandler("OpenService�ŃG���[������", GetLastError());
	}


	// �K�v�Ȃ�T�[�r�X���~����
	success = QueryServiceStatus(service, &status);
	if (!success)
	{
		ErrorHandler("QueryServiceStatus�ŃG���[������", GetLastError());
	}

	if (status.dwCurrentState != SERVICE_STOPPED)
	{
		cout << "�T�[�r�X���~���܂�...\n";

		success = ControlService(service, SERVICE_CONTROL_STOP, &status);
		if (!success)
		{
			ErrorHandler("ControlService�ŃG���[������", GetLastError());
		}
		Sleep(5000);
	}

	// �T�[�r�X���폜����
	success = DeleteService(service);
	if (!success)
	{
		ErrorHandler("DeleteService�ŃG���[������", GetLastError());
	}

	cout << "�T�[�r�X���폜���܂���\n";

	// �N���[���A�b�v
	CloseServiceHandle(service);
	CloseServiceHandle(scm);

	cout << "�폜���I�����܂�...\n";
	return 0;
}




