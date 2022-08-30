#ifdef __GNUC__
///////////////////////////////////////////////////////////////////////////////
//  Start Command
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <getopt.h>
#include <mqueue.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include "TcpCommon.h"
#include <netdb.h>
using namespace std;

int fdStart = -1;//立ち上がり完了報告用fd;
int nByte;
char buff[256];
int nRet;

int closeStartPipe();
void eraseTail(char *str, int n);
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	const char* name = "eventserver";
	const char* protocol = "tcp";

	struct servent* pServent = NULL;
	pServent = getservbyname(name, protocol);
	int port = 5000;//Default value
	char portBuff[16];

	if (pServent == NULL) 
	{
		std::sprintf(portBuff, "%d", port);
		printf("getservbyname error, uses %s for Port.\n", portBuff);
	}else {
		port = ntohs(pServent->s_port);
		std::sprintf(portBuff, "%d", port);
		printf("Uses %s for Port.\n", portBuff);
	}

	// 入力パイプの作成
	unlink(PIPE_START);
	nRet = mkfifo(PIPE_START, 0666);
	if ( nRet ==-1 ) {
		perror("mkfifo\n");
		return -1;
	}
	
	//エラー判定値(int)
	int nRet = 0;

	////////////////////////
	//TcpServerプロセス起動
	////////////////////////

	//StartコマンドプロセスID取得
	pid_t rPid = getpid();
	char pidBuff[16];
    //char portBuff[16];

	//strcpy(portBuff, argv[1]);
    sprintf(pidBuff, "%d", rPid);//文字列に変換

	// getcwd(CurrentPath, 256);

	char tcpServerPath[256];//デーモン実行するサーバープログラムのパス
	char symbolicLink[256];//実行中のプロセスのシンボリックリンク
	char absolutePath[256];//実行中のプログラム絶対パス
	memset(tcpServerPath, 0, sizeof(tcpServerPath));
	memset(symbolicLink, 0, sizeof(symbolicLink));
	memset(absolutePath, 0, sizeof(absolutePath));

	snprintf(symbolicLink, sizeof(symbolicLink)-1, "/proc/%d/exe", rPid);
	readlink(symbolicLink, absolutePath, sizeof(absolutePath));

	eraseTail(absolutePath, sizeof("Start"));
	// printf("absolutePath = [%s]\n", absolutePath);
	sprintf(tcpServerPath, "%s%s",absolutePath, "/../build/ServerMain");

	char* const str[] = {(char*)"myServer", portBuff, pidBuff, NULL};
	pid_t pid = 0;
	
	pid = fork();

    if (pid == -1 ) 
	{
        printf("fork has failed in Start.cpp\n");
		return -1;
    }
	else if (pid == 0) //子プロセスには0が返る
	{
		//ToDo:renameat関数使ってリネームできそう
		printf("tcpServerPath = [%s]\n", tcpServerPath);
		nRet = execv(tcpServerPath, str);
        if ( nRet == -1 ) 
		{
			printf("execv has failed in Start.cpp. errno=%d\n", errno);
			return -1;
        }
    }

	//TcpServerプロセスから正常起動完了報告を名前付きパイプで受ける
    /* 読取専用でパイプを開く */
    if ((fdStart = open(PIPE_START, O_RDONLY)) == -1)
    {
		closeStartPipe();
        return 1;
    }

    //TcpServer立ち上がり完了報告を読取
    bool flag = true;

    while (flag == true){
        if((nByte = read(fdStart, buff, sizeof(buff))) > 0){
            write(fileno(stdout), buff, nByte);
			printf("\n");
            flag = false;
        }else if(nByte == -1){
			perror("write()\n");
			closeStartPipe();
			return -1;
		}
    }

	closeStartPipe();
	return 0;
}

int closeStartPipe(){
	if(close(fdStart) != 0) perror("close()\n");
    unlink(PIPE_START);
	return(0);
};

void eraseTail(char *str, int n)
{
	const int len = strlen(str);
	if (n >= len) 
	{
		*str = '\0';
	}
	else {
		str[len - n] = '\0';
	}
}
#endif
//スネークではなくキャメルケース
//頭は小文字
//定数は全部大文字
//動詞目的語
//意味を持った変数名
//マジックナンバーはやめて定数名をつける