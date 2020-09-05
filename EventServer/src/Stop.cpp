///////////////////////////////////////////////////////////////////////////////
// [stop]
//  �@�\ : TcpServer�ɖ��O�t���p�C�v�āK���b�Z�[�W�𑗐M����B
//  �\�� : stop ���M���b�Z�[�W
///////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// DEFINE
///////////////////////////////////////////////////////////////////////////////
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	//�����`�F�b�N
	if (argc != 2) {
		printf("Usage : Stop Message\n");
		return -1;
	}

	// �R���s���[�^�[���̎擾	
	char machineName[80];
	DWORD bufflen;
	GetComputerName((LPSTR)machineName, &bufflen);

	// �p�C�v���̑g�ݗ���
	char pipeName[80];
	wsprintf(pipeName, PIPE_NAME, machineName);

	// �T�[�o�̃p�C�v�ɐڑ�
	HANDLE hPipe = CreateFile(pipeName,
							GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
							(LPSECURITY_ATTRIBUTES)NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							(HANDLE)NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	//�@�f�[�^�̑��M
	BOOL bRet;
	DWORD NumBytesWritten;
	bRet = WriteFile(hPipe,
						argv[1],
						(DWORD)strlen(argv[1]) + 1,
						&NumBytesWritten,
						(LPOVERLAPPED)NULL);
	if ( bRet != TRUE ) {
		printf("WriteFile error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
		return -1;
	}

	// �T�[�o����̉�������M
	DWORD NumBytesRead;
	char buf[1024];
	bRet = ReadFile(hPipe,
					buf,
					sizeof(buf),
					&NumBytesRead,
					(LPOVERLAPPED)NULL);
	if (bRet != TRUE) {
		printf("ReadFile error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
		return -1;
	}

	printf("�T�[�o����̉��� : [%s]\n", buf);

	bRet = CloseHandle(hPipe);
	if (bRet != TRUE) {
		printf("CloseHandle error.(%ld)\n", GetLastError());
		CloseHandle(hPipe);
	}

	return 0;
}