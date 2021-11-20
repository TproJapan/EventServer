///////////////////////////////////////////////////////////////////////////////
// �T�[�r�X�̃A���C���X�g�[���v���O�����T���v��
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// �v���g�^�C�v�錾
///////////////////////////////////////////////////////////////////////////////
void ErrorHandler(const char* s, DWORD err);


#define SERVICE_NAME  L"SampleService"
#define SERVICE_DIPLAY_NAME  L"SampleServ"
#define SERVICE_DETAIL  L"�e�X�g�p�̃T�[�r�X�ł�"
//#define SERVICE_EXE_FILEPATH  L"C:\\Users\\zx9y-\\source\\repos\\SampleService\\x64\\Debug\\SampleService.exe"
#define SERVICE_EXE_FILEPATH  L"C:\\Users\\suguru.tateda\\VSProjects\\SampleService211107\\x64\\Debug\\SampleService.exe"

///////////////////////////////////////////////////////////////////////////////
// ���C������
// uninstall SampleService
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	SC_HANDLE	service;
	SC_HANDLE	scm;
	BOOL		success;
	SERVICE_STATUS	status;

	cout << "�폜���J�n���܂�...\n";

	// SCM�ւ̐ڑ����J��
	scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
	{
		ErrorHandler("OpenSCManager�ŃG���[������", GetLastError());
		return -1;
	}

	// �T�[�r�X�̃n���h�����擾����
	service = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS | DELETE);
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

///////////////////////////////////////////////////////////////////////////////
// �G���[�n���h���[
///////////////////////////////////////////////////////////////////////////////
void ErrorHandler(const char* s, DWORD err)
{
	cout << s << endl;
	cout << "�G���[�ԍ�: " << err << endl;
	ExitProcess(err);
}
