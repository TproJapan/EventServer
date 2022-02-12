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
ConnectClient::ConnectClient(SOCKET& tmpsocket)
{
	_socket = tmpsocket;
	//_pSocketMap = tmpSocketMapPtr;
	_live = true;
}

///////////////////////////////////////////////////////////////////////////////
// デストラクタ
///////////////////////////////////////////////////////////////////////////////
ConnectClient::~ConnectClient()
{
	if (_socket != INVALID_SOCKET) closesocket(_socket);
}

///////////////////////////////////////////////////////////////////////////////
//ハンドラ制御
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::func()
{
	HANDLE tmpEvent = WSACreateEvent();

	//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
	int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		//printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		write_log(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
		return;
	}

	//_pSocketMap->addSocket(_socket, tmpEvent);

	//イベントを配列で管理
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//サーバーステータスチェック
		if (checkServerStatus() == 1) break;

		//強制終了用のinterruption_pointを張る
		boost::this_thread::interruption_point();

		//printf("書き込みを待っています.\n");
		write_log(2, "書き込みを待っています.,%s %d %s\n", __FILENAME__ , __LINE__, __func__);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//イベント多重待ち
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
                                                   workerEventList,
                                                   FALSE,
			                                       worker_dwTimeout,
			                                       FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			//printf("WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
			write_log(5, "WSAWaitForMultipleEvents wait error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			//printf("タイムアウト発生です!!!\n");
			write_log(2, "タイムアウト発生です!!!, %s %d %s\n", __FILENAME__ , __LINE__, __func__);
			continue;
		}

		//printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);
		write_log(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", worker_nRet, __FILENAME__ , __LINE__, __func__);

		// イベントを検知したHANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//イベント調査
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			//printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__ , __LINE__, __func__);
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// クライアントとの通信ソケットにデータが到着した
			//recvHandler(*pSocketMap, workerHandle);
			recvHandler(workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// クライアントとの通信ソケットのクローズを検知
			//closeHandler(*pSocketMap, workerHandle);
			closeHandler(workerHandle);
		}
	}

	//deleteConnection(*pSocketMap, tmpEvent);
	//closesocket(_socket); // konishi
	//CloseHandle(tmpEvent);// konishi
#if 1
	if (_socket != INVALID_SOCKET) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (tmpEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(tmpEvent);
		tmpEvent = INVALID_HANDLE_VALUE;
	}
#endif

	//delete this;

	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;

	//printf("Finished Thead\n");
	write_log(2, "Finished Thead, %s %d %s\n", __FILENAME__ , __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
//bool ConnectClient::recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
bool ConnectClient::recvHandler(HANDLE& hEvent)
{
	//SOCKET sock = socketMap.getSocket(hEvent);
	//printf("クライアント(%ld)からデータを受信\n", sock);
	write_log(2, "クライアント(%ld)からデータを受信, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);

	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		//printf("recv error.\n");
		write_log(4, "recv error., %s %d %s\n", __FILENAME__ , __LINE__, __func__);
		//printf("クライアント(%ld)との接続が切れました\n", sock);
		write_log(4, "クライアント(%ld)との接続が切れました, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	//printf("変換前:[%s] ==> ", buf);
	write_log(2, "変換前:[%s] ==> %s %d %s\n", buf, __FILENAME__ , __LINE__, __func__);

	for (int i = 0; i < (int)stSize; i++) { // bufの中の小文字を大文字に変換
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// クライアントに返信
	stSize = send(_socket, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		//printf("send error.\n");
		write_log(4, "send error., %s %d %s\n", __FILENAME__ , __LINE__, __func__);
		//printf("クライアントとの接続が切れました\n");
		write_log(4, "クライアントとの接続が切れました, %s %d %s\n", __FILENAME__ , __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	//printf("変換後:[%s] \n", buf);
	write_log(2, "変換後:[%s] %s %d %s\n", buf, __FILENAME__ , __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントとの通信ソケットの切断検知時のハンドラ
///////////////////////////////////////////////////////////////////////////////
//bool ConnectClient::closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
bool ConnectClient::closeHandler(HANDLE& hEvent)
{
	//SOCKET sock = socketMap.getSocket(hEvent);

	//printf("クライアント(%d)との接続が切れました\n", sock);
	write_log(4, "クライアント(%d)との接続が切れました, %s %d %s\n", _socket, __FILENAME__ , __LINE__, __func__);
	deleteConnection(hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ
///////////////////////////////////////////////////////////////////////////////
//void ConnectClient::deleteConnection(CSocketMap& socketMap, HANDLE& hEvent)
void ConnectClient::deleteConnection(HANDLE& hEvent)
{
	//socketMap.deleteSocket(hEvent);
	//closesocket(_socket); // konishi
	//CloseHandle(hEvent);  // konishi

	if (_socket != INVALID_SOCKET) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	if (hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(hEvent);
		hEvent = INVALID_HANDLE_VALUE;
	}

	return;
}