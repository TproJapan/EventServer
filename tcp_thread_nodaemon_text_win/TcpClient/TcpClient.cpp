///////////////////////////////////////////////////////////////////////////////
//  TcpServer
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>
#pragma warning(disable:4996)// inet_addr()関数で警告が出る場合は以下で警告を無効化
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdlib.h>
#include <errno.h>
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    ///////////////////////////////////
    // コマンド引数の解析
    ///////////////////////////////////
    if (argc != 3) {
        printf("TcpClient IPAddress portNo\n");
        return -1;
    }

    int nRet;
    char destination[32];   // 送信先IPアドレス
    int nPortNo;            // ポート番号
    strcpy(destination, argv[1]);
    nPortNo = atol(argv[2]);

#ifdef _WIN32
    // WinSockの初期化
    DWORD dwRet = 0;
    WSADATA wsaData;
    nRet = WSAStartup(MAKEWORD(2, 0), &wsaData);
    if (nRet != 0) {
        printf("WSAStartup error\n");
        return(-1);
    }
#endif

    ///////////////////////////////////
    //sockaddr_in 構造体の作成
    ///////////////////////////////////
    struct sockaddr_in dstAddr;
    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(nPortNo);
    dstAddr.sin_addr.s_addr = inet_addr(destination);

    ///////////////////////////////////
    //空のソケットの生成
    ///////////////////////////////////
#ifdef _WIN32
    SOCKET dstSocket;
#else
    int dstSocket;
#endif
    dstSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dstSocket == -1) {
        printf("ソケットの作成に失敗\n");
        return(-1);
    }

    ///////////////////////////////////
    //空のソケットにsockaddr_in構造体をbindしてサーバに接続
    ///////////////////////////////////
    nRet = connect(dstSocket,
        (struct sockaddr*)&dstAddr,
        sizeof(dstAddr));
    if (nRet == -1) {
        dwRet = WSAGetLastError();
        printf("%s に接続できませんでした. dwRet=%d\n", destination, dwRet);
        return(-1);
    }
    printf("%s に接続しました\n", destination);

    ///////////////////////////////////
    //　サーバとの通信
    ///////////////////////////////////
    while (1) {
        // 送信するデータの読み込み
        char   buf[1024];
        printf("子文字のアルファベットを入力してください\n");
        scanf("%s", buf);

        if (strcmp(buf, ".") == 0) {
            printf("処理を終了します\n");
            break;
        }

        //　データの送信
        size_t stSize;
        stSize = send(dstSocket,
            buf,
            strlen(buf) + 1,
            0);
        if (stSize != strlen(buf) + 1) {
            printf("send error.\n");
            break;
        }

        // サーバからの応答を受信
        stSize = recv(dstSocket,
            buf,
            sizeof(buf),
            0);
        if (stSize <= 0) {
            dwRet = WSAGetLastError();
            printf("recv error.　errno = %d\n", dwRet);//エラーナンバー(http://chokuto.ifdef.jp/advanced/prm/winsock_error_code.html)
            printf("stSize = %d\n", (int)stSize);//stSizeが0ならソケットが切れたと言う事。失敗はだいたいstSizeが-1だとerrnoは4とか
            break;
        }
        printf("→ %s\n", buf);
    }

#ifdef _WIN32
    closesocket(dstSocket);
#else
    close(dstSocket);
#endif

#ifdef _WIN32
    // WinSockの終了
    WSACleanup();
#endif
    return(0);
}