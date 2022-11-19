#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN64
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
typedef int SOCKET;
#else
#ifdef _WIN64
#pragma warning(disable:4996)
#include <WinSock2.h>
#endif
#include <stdlib.h>
#include <errno.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    char messageBuf[1024];

    ///////////////////////////////////
    // Analyze command-line argument
    ///////////////////////////////////
    if (argc != 3) {
        printf("TcpClient IPAddress portNo\n");
        return -1;
    }

    char destination[32];   // IP Address
    int nPortNo;            // Port Number
    strcpy(destination, argv[1]);
    nPortNo = atol(argv[2]);

    ///////////////////////////////////
    //Create sockaddr_in Structure
    ///////////////////////////////////
    struct sockaddr_in dstAddr;
    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(nPortNo);
    dstAddr.sin_addr.s_addr = inet_addr(destination);

#ifdef _WIN64
    WSADATA	WsaData;
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD(2, 0);
    if (WSAStartup(wVersionRequested, &WsaData) != 0) {
        //printf("WSAStartup() error. code=%d\n", WSAGetLastError());
        printf("%ld:%s", WSAGetLastError(), "WSAStartup() error.\n");
        return -1;
    }
#endif
    ///////////////////////////////////
    //Create empty socket
    ///////////////////////////////////
    SOCKET dstSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dstSocket == -1) {
        //printf("Failed to create socket\n");
        printf("%ld:%s", WSAGetLastError(), "Failed to create socket\n");
        return(-1);
    }

    ///////////////////////////////////////////////////////
    //Bind sockaddr_in Structure to the empty socket,
    //Connect server
    ///////////////////////////////////////////////////////
    int nRet;
    nRet = connect(dstSocket,
        (struct sockaddr*)&dstAddr,
        sizeof(dstAddr));
    if (nRet == -1) {
        //printf("Fialed to connect to %s\n", destination);
        sprintf(messageBuf, "Fialed to connect to %s\n", destination);
        printf("%ld:%s", WSAGetLastError(), messageBuf);
        return(-1);
    }
    printf("Succeed to connect to %s\n", destination);

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
            //printf("send error.\n");
            printf("%ld:%s", errno, "send error.\n");
            break;
        }

        // サーバからの応答を受信
        stSize = recv(dstSocket,
            buf,
            sizeof(buf),
            0);
        if (stSize <= 0) {
            //printf("recv error.　errno = %d\n", errno);//エラーナンバーが0なら異常終了
            //printf("stSize = %d\n", (int)stSize);//stSizeが0ならソケットが切れたと言う事。失敗はだいたいstSizeが-1だとerrnoは4とか
            sprintf(messageBuf, "recv error.\nstSize = %d\n", (int)stSize);
            printf("%ld:%s", errno, messageBuf);

            break;
        }
        printf("→ %s\n", buf);
    }

    // ソケットをクローズ
#ifdef _WIN64
    closesocket(dstSocket);
#else
    close(dstSocket);
#endif
    return(0);
}