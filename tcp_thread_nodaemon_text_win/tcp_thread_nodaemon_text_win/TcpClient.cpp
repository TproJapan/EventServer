///////////////////////////////////////////////////////////////////////////////
//  TcpServer
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#pragma warning(disable:4996)
#include <WinSock2.h>
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
    // �R�}���h�����̉��
    ///////////////////////////////////
    if (argc != 3) {
        printf("TcpClient IPAddress portNo\n");
        return -1;
    }

    char destination[32];   // ���M��IP�A�h���X
    int nPortNo;            // �|�[�g�ԍ�
    strcpy(destination, argv[1]);
    nPortNo = atol(argv[2]);

    ///////////////////////////////////
    //sockaddr_in �\���̂̍쐬
    ///////////////////////////////////
    struct sockaddr_in dstAddr;
    memset(&dstAddr, 0, sizeof(dstAddr));
    dstAddr.sin_family = AF_INET;
    dstAddr.sin_port = htons(nPortNo);
    dstAddr.sin_addr.s_addr = inet_addr(destination);

    ///////////////////////////////////
    //��̃\�P�b�g�̐���
    ///////////////////////////////////
    int dstSocket;
    dstSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dstSocket == -1) {
        printf("�\�P�b�g�̍쐬�Ɏ��s\n");
        return(-1);
    }


    ///////////////////////////////////////////////////////
    //��̃\�P�b�g��sockaddr_in�\���̂�bind���ăT�[�o�ɐڑ�
    ///////////////////////////////////////////////////////
    int nRet;
    nRet = connect(dstSocket,
        (struct sockaddr*)&dstAddr,
        sizeof(dstAddr));
    if (nRet == -1) {
        printf("%s �ɐڑ��ł��܂���ł���\n", destination);
        return(-1);
    }
    printf("%s �ɐڑ����܂���\n", destination);


    ///////////////////////////////////
    //�@�T�[�o�Ƃ̒ʐM
    ///////////////////////////////////
    while (1) {
        // ���M����f�[�^�̓ǂݍ���
        char   buf[1024];
        printf("�q�����̃A���t�@�x�b�g����͂��Ă�������\n");
        scanf("%s", buf);

        if (strcmp(buf, ".") == 0) {
            printf("�������I�����܂�\n");
            break;
        }

        //�@�f�[�^�̑��M
        size_t stSize;
        stSize = send(dstSocket,
            buf,
            strlen(buf) + 1,
            0);
        if (stSize != strlen(buf) + 1) {
            printf("send error.\n");
            break;
        }

        // �T�[�o����̉�������M
        stSize = recv(dstSocket,
            buf,
            sizeof(buf),
            0);
        if (stSize <= 0) {
            printf("recv error.�@errno = %d\n", errno);//�G���[�i���o�[��0�Ȃ�ُ�I��
            printf("stSize = %d\n", (int)stSize);//stSize��0�Ȃ�\�P�b�g���؂ꂽ�ƌ������B���s�͂�������stSize��-1����errno��4�Ƃ�
            break;
        }
        printf("�� %s\n", buf);
    }

    // �\�P�b�g���N���[�Y
#ifdef _WIN32
#else
    close(dstSocket);
#endif
    return(0);
}