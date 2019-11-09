///////////////////////////////////////////////////////////////////////////////
//  TcpServer
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    ///////////////////////////////////
    // コマンド引数の解析
    ///////////////////////////////////
    if ( argc != 3 ) {
      printf("TcpClient IPAddress portNo\n");
      return -1;
    }

    char destination[32];   // 送信先IPアドレス
    int nPortNo;            // ポート番号
    strcpy(destination, argv[1]);
    nPortNo = atol(argv[2]);

    ///////////////////////////////////
    //sockaddr_in 構造体の作成
    ///////////////////////////////////
    struct sockaddr_in dstAddr;
    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port   = htons(nPortNo);
    dstAddr.sin_addr.s_addr = inet_addr(destination);

    ///////////////////////////////////
    //空のソケットの生成
    ///////////////////////////////////
    int dstSocket;
    dstSocket = socket(AF_INET, SOCK_STREAM, 0);
    if ( dstSocket == -1 ) {
        printf("ソケットの作成に失敗\n");
        return(-1);      
    }


    ///////////////////////////////////////////////////////
    //空のソケットにsockaddr_in構造体をbindしてサーバに接続
    ///////////////////////////////////////////////////////
    int nRet;
    nRet = connect(dstSocket,
                   (struct sockaddr *)&dstAddr,
                   sizeof(dstAddr));
    if ( nRet == -1 ) {
        printf("%s に接続できませんでした\n",destination);
        return(-1);
    }
    printf("%s に接続しました\n",destination);


    ///////////////////////////////////
    //　サーバとの通信
    ///////////////////////////////////
    while (1) {
        // 送信するデータの読み込み
        char   buf[1024];
        printf("子文字のアルファベットを入力してください\n");
        scanf("%s",buf);
        
        if ( strcmp(buf, ".") == 0) {
            printf("処理を終了します\n");
            break;
        }

        //　データの送信
        size_t stSize;
        stSize = send(dstSocket,
                      buf,
                      strlen(buf)+1,
                      0);
        if ( stSize != strlen(buf)+1) {
          printf("send error.\n");
          break;
        }

        // サーバからの応答を受信
        stSize = recv(dstSocket,
                      buf,
                      sizeof(buf),
                      0);
        if ( stSize <= 0 ) {
          printf("recv error.\n");
          break;
        }
        printf("→ %s\n",buf);
    }

    // ソケットをクローズ
    close(dstSocket);

    return(0);
}


