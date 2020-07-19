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
	SC_HANDLE	newService;
	SC_HANDLE	scm;

	if (argc != 4)
	{
		cout << "�g�p���@:\n";
		cout << "  install <servicename> <service_label> <executable>\n";
		return -1;
	}

	cout << "�C���X�g�[�����J�n���܂�...\n";

	// SCM�ւ̐ڑ����J��
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		ErrorHandler("OpenSCManager�ŃG���[������", GetLastError());
	}

	cout << "�T�[�r�X�� : " << argv[1] << endl;
	cout << "�T�[�r�X�̕\���� : " << argv[2] << endl;
	cout << "���s�t�@�C���̃p�X�� : " << argv[3] << endl;

	// �V�����T�[�r�X���C���X�g�[������
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
		ErrorHandler("CreateService�ŃG���[������", GetLastError());
	}

	cout << "�T�[�r�X�̓C���X�g�[������܂���\n";

	// �N���[���A�b�v
	CloseServiceHandle(newService);
	CloseServiceHandle(scm);

	cout << "�C���X�g�[�����I�����܂�...\n";
	return 0;
}




