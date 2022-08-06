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

int fd_start = -1;//立ち上がり完了報告用fd;
int nbyte;
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
	char port_buff[16];

	if (pServent == NULL) 
	{
		std::sprintf(port_buff, "%d", port);
		printf("getservbyname error, uses %s for Port.\n", port_buff);
	}else {
		port = ntohs(pServent->s_port);
		std::sprintf(port_buff, "%d", port);
		printf("Uses %s for Port.\n", port_buff);
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
	pid_t r_pid = getpid();
	char pid_buff[16];
    //char port_buff[16];

	//strcpy(port_buff, argv[1]);
    sprintf(pid_buff, "%d", r_pid);//文字列に変換

	// getcwd(CurrentPath, 256);

	char TcpServerPath[256];//デーモン実行するサーバープログラムのパス
	char SymbolicLink[256];//実行中のプロセスのシンボリックリンク
	char AbsolutePath[256];//実行中のプログラム絶対パス
	memset(TcpServerPath, 0, sizeof(TcpServerPath));
	memset(SymbolicLink, 0, sizeof(SymbolicLink));
	memset(AbsolutePath, 0, sizeof(AbsolutePath));

	snprintf(SymbolicLink, sizeof(SymbolicLink)-1, "/proc/%d/exe", r_pid);
	readlink(SymbolicLink, AbsolutePath, sizeof(AbsolutePath));

	eraseTail(AbsolutePath, sizeof("Start"));
	// printf("AbsolutePath = [%s]\n", AbsolutePath);
	sprintf(TcpServerPath, "%s%s",AbsolutePath, "/../build/ServerMain");

	char* const str[] = {(char*)"myServer", port_buff, pid_buff, NULL};
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
		printf("TcpServerPath = [%s]\n", TcpServerPath);
		nRet = execv(TcpServerPath, str);
        if ( nRet == -1 ) 
		{
			printf("execv has failed in Start.cpp. errno=%d\n", errno);
			return -1;
        }
    }

	//TcpServerプロセスから正常起動完了報告を名前付きパイプで受ける
    /* 読取専用でパイプを開く */
    if ((fd_start = open(PIPE_START, O_RDONLY)) == -1)
    {
		closeStartPipe();
        return 1;
    }

    //TcpServer立ち上がり完了報告を読取
    bool flag = true;

    while (flag == true){
        if((nbyte = read(fd_start, buff, sizeof(buff))) > 0){
            write(fileno(stdout), buff, nbyte);
			printf("\n");
            flag = false;
        }else if(nbyte == -1){
			perror("write()\n");
			closeStartPipe();
			return -1;
		}
    }

	closeStartPipe();
	return 0;
}

int closeStartPipe(){
	if(close(fd_start) != 0) perror("close()\n");
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