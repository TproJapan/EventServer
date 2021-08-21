#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include "CommonVariables.h"
#include "CommonFunc.h"
#include <boost/asio.hpp>
#include "CommonVariables.h"
#include "BoostLog.h"


///////////////////////////////////////////////////////////////////////////////
// コンストラクタ
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr)
{
	_socket = tmpsocket;
	_pSocketMap = tmpSocketMapPtr;
}

///////////////////////////////////////////////////////////////////////////////
// デストラクタ
///////////////////////////////////////////////////////////////////////////////
ConnectClient::~ConnectClient()
{

}

///////////////////////////////////////////////////////////////////////////////
//ハンドラ制御
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::func()
{
	char log_buff[1024];

	HANDLE tmpEvent = WSACreateEvent();

	//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
	int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		sprintf(log_buff, "WSAEventSelect error. (%ld)\n", WSAGetLastError());
		write_log(5,log_buff);
		return;
	}

	_pSocketMap->addSocket(_socket, tmpEvent);

	//イベントを配列で管理
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//サーバーステータスチェック
		if (checkServerStatus() == 1) break;

		//強制終了用のinterruption_pointを張る
		boost::this_thread::interruption_point();

		printf("書き込みを待っています.\n");
		sprintf(log_buff, "書き込みを待っています.\n");
		write_log(2,log_buff);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//イベント多重待ち
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
                                                   workerEventList,
                                                   FALSE,
			                                       worker_dwTimeout,
			                                       FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			sprintf(log_buff, "WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			write_log(5,log_buff);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			printf("タイムアウト発生です!!!\n");
			sprintf(log_buff, "タイムアウト発生です!!!\n");
			write_log(2,log_buff);
			continue;
		}

		printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		sprintf(log_buff, "WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		write_log(2,log_buff);

		// イベントを検知したHANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//イベント調査
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			sprintf(log_buff, "WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			write_log(5,log_buff);
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// クライアントとの通信ソケットにデータが到着した
			recvHandler(*pSocketMap, workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// クライアントとの通信ソケットのクローズを検知
			closeHandler(*pSocketMap, workerHandle);
		}
	}

	deleteConnection(*pSocketMap, tmpEvent);
	delete this;
	printf("Finished Thead\n");
	sprintf(log_buff, "Finished Thead\n");
	write_log(2,log_buff);
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	char log_buff[1024];
	SOCKET sock = socketMap.getSocket(hEvent);
	printf("クライアント(%ld)からデータを受信\n", sock);
	sprintf(log_buff, "クライアント(%ld)からデータを受信\n", sock);
	write_log(2,log_buff);

	char buf[1024];
	int stSize = recv(sock, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		printf("recv error.\n");
		sprintf(log_buff, "recv error.\n");
		write_log(4,log_buff);
		printf("クライアント(%ld)との接続が切れました\n", sock);
		sprintf(log_buff, "クライアント(%ld)との接続が切れました\n", sock);
		write_log(4,log_buff);

		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("変換前:[%s] ==> ", buf);
	sprintf(log_buff, "変換前:[%s] ==> ", buf);
	write_log(2,log_buff);

	for (int i = 0; i < (int)stSize; i++) { // bufの中の小文字を大文字に変換
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// クライアントに返信
	stSize = send(sock, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		printf("send error.\n");
		sprintf(log_buff, "send error.\n");
		write_log(4,log_buff);
		printf("クライアントとの接続が切れました\n");
		sprintf(log_buff, "クライアントとの接続が切れました\n");
		write_log(4,log_buff);

		deleteConnection(socketMap, hEvent);
		return true;
	}

	printf("変換後:[%s] \n", buf);
	sprintf(log_buff, "変換後:[%s] \n", buf);
	write_log(2,log_buff);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントとの通信ソケットの切断検知時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
{
	char log_buff[1024];
	SOCKET sock = socketMap.getSocket(hEvent);

	printf("クライアント(%d)との接続が切れました\n", sock);
	sprintf(log_buff, "クライアント(%d)との接続が切れました\n", sock);
	write_log(4,log_buff);
	deleteConnection(socketMap, hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ、mapからの削除
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::deleteConnection(CSocketMap& socketMap, HANDLE& hEvent)
{
	socketMap.deleteSocket(hEvent);
	return;
}