#ifdef __GNUC__
#include "ConnectClient.h"
#include "BoostLog.h"
#include "TcpCommon.h"
#include <thread>
#include <sys/socket.h>
///////////////////////////////////////////////////////////////////////////////
// コンストラクタ
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(int dstSocket)
{
	_dstSocket = dstSocket;
	_live = true;
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
	write_log(2, "func started. _dstSocket=%d, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);

	while (1) {
		//終了確認
		if (GetServerStatus() == 1) {
			write_log(2, "Thread:%sを終了します, %s %d %s\n", std::this_thread::get_id(), __FILENAME__, __LINE__, __func__);
			break;
		}

		///////////////////////////////////
		// 通信
		///////////////////////////////////
		write_log(2, "client(%d)クライアントとの通信を開始します, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);
		size_t stSize;
		char buf[1024];

		// タイムアウトの設定
		struct timeval  tval;
		tval.tv_sec = SELECT_TIMER_SEC;	// time_t  秒
		tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒

		fd_set  readfds;//ビットフラグ管理変数
		FD_ZERO(&readfds);//初期化

		FD_SET(_dstSocket, &readfds);

		int nRet = select(FD_SETSIZE,
			&readfds,
			NULL,
			NULL,
			&tval);

		if (nRet == -1) {
			if (errno == EINTR) {//シグナル割り込みは除外
				continue;
			}
			else {
				// selectが異常終了
				write_log(4, "select error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
				break;
			}
		}
		else if (nRet == 0) {
			write_log(2, "workerスレッドでタイムアウト発生, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		stSize = recv(_dstSocket,
			buf,
			sizeof(buf),
			0);
		if (stSize <= 0) {
			write_log(4, "recv error, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			write_log(4, "クライアント(%d)との接続が切れました, %s %d %s\n", _dstSocket, __FILENAME__, __LINE__, __func__);
			close(_dstSocket);
			break;
		}

		write_log(2, "変換前:[%s] ==> , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
		for (int i = 0; i < stSize; i++) { // bufの中の小文字を大文字に変換
			if (isalpha(buf[i])) {
				buf[i] = toupper(buf[i]);
			}
		}

		// クライアントに返信
		stSize = send(_dstSocket,
			buf,
			strlen(buf) + 1,
			0);

		if (stSize != strlen(buf) + 1) {
			write_log(4, "send error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
			write_log(4, "クライアントとの接続が切れました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			close(_dstSocket);
			break;
		}
		write_log(2, "変換後:[%s] , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	}
	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;
}
#else


#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include "TcpCommon.h"
#include <boost/asio.hpp>
#include "BoostLog.h"


///////////////////////////////////////////////////////////////////////////////
// コンストラクタ
///////////////////////////////////////////////////////////////////////////////
ConnectClient::ConnectClient(SOCKET& tmpsocket)
{
	_socket = tmpsocket;
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
		write_log(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		return;
	}

	//イベントを配列で管理
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//サーバーステータスチェック
		if (checkServerStatus() == 1) break;

		//強制終了用のinterruption_pointを張る
		boost::this_thread::interruption_point();

		write_log(2, "書き込みを待っています.,%s %d %s\n", __FILENAME__, __LINE__, __func__);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//イベント多重待ち
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
			workerEventList,
			FALSE,
			worker_dwTimeout,
			FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			write_log(5, "WSAWaitForMultipleEvents wait error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			write_log(2, "タイムアウト発生です!!!, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		write_log(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", worker_nRet, __FILENAME__, __LINE__, __func__);

		// イベントを検知したHANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//イベント調査
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			write_log(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			break;
		}

		//READ
		if (events.lNetworkEvents & FD_READ)
		{
			// クライアントとの通信ソケットにデータが到着した
			recvHandler(workerHandle);
		}

		//CLOSE
		if (events.lNetworkEvents & FD_CLOSE)
		{
			// クライアントとの通信ソケットのクローズを検知
			closeHandler(workerHandle);
		}
	}

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

	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;

	write_log(2, "Finished Thead, %s %d %s\n", __FILENAME__, __LINE__, __func__);
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::recvHandler(HANDLE& hEvent)
{
	write_log(2, "クライアント(%ld)からデータを受信, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		write_log(4, "recv error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		write_log(4, "クライアント(%ld)との接続が切れました, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	write_log(2, "変換前:[%s] ==> %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);

	for (int i = 0; i < (int)stSize; i++) { // bufの中の小文字を大文字に変換
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// クライアントに返信
	stSize = send(_socket, buf, strlen(buf) + 1, 0);
	if (stSize != strlen(buf) + 1) {
		write_log(4, "send error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		write_log(4, "クライアントとの接続が切れました, %s %d %s\n", __FILENAME__, __LINE__, __func__);

		deleteConnection(hEvent);
		return true;
	}

	write_log(2, "変換後:[%s] %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントとの通信ソケットの切断検知時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(HANDLE& hEvent)
{
	write_log(4, "クライアント(%d)との接続が切れました, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
	deleteConnection(hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::deleteConnection(HANDLE& hEvent)
{
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
#endif