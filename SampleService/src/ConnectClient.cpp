#include "BoostLog.h"
#include "TcpCommon.h"
#ifndef _WIN64
#include "ConnectClient.h"
#include <thread>
#include <sys/socket.h>
#else
#pragma warning(disable:4996)
#include <WinSock2.h>
#include "ConnectClient.h"
#include <boost/asio.hpp>
#endif

#ifndef _WIN64
#define INVALID_SOCKET (-1)
#endif

ConnectClient::~ConnectClient()
{
	write_log(2, "destructor started. _socket=%d, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);//konishi
	if (_socket != INVALID_SOCKET) 
	{
#ifdef _WIN64
	closesocket(_socket);
#else
	close(_socket);
#endif
	}
}

ConnectClient::ConnectClient(SOCKET dstSocket)
{
	write_log(2, "constructor started. dstSocket=%d, %s %d %s\n", dstSocket, __FILENAME__, __LINE__, __func__);//konishi
	_socket = dstSocket;
	_live = true;
}


///////////////////////////////////////////////////////////////////////////////
//ハンドラ制御
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::func()
{
#ifndef _WIN64
	write_log(2, "func started. _socket=%d, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

	while (1) {
		//終了確認
		if (GetServerStatus() == 1) {
			write_log(2, "Thread:%sを終了します, %s %d %s\n", std::this_thread::get_id(), __FILENAME__, __LINE__, __func__);
			break;
		}

		///////////////////////////////////
		// 通信
		///////////////////////////////////
		write_log(2, "client(%d)クライアントとの通信を開始します, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
		size_t stSize;
		char buf[1024];

		// タイムアウトの設定
		struct timeval  tval;
		tval.tv_sec = SELECT_TIMER_SEC;	// time_t  秒
		tval.tv_usec = SELECT_TIMER_USEC;	// suseconds_t  マイクロ秒

		fd_set  readfds;//ビットフラグ管理変数
		FD_ZERO(&readfds);//初期化

		FD_SET(_socket, &readfds);

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
				write_log(4, "select error. errno=%d, %s %d %s\n", errno, __FILENAME__, __LINE__, __func__);// konishi
				break;
			}
		}
		else if (nRet == 0) {
			write_log(2, "workerスレッドでタイムアウト発生, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		// クライアントとの通信ソケットにデータが到着した
		recvHandler();
	}
	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;
#else
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
		if (GetServerStatus() == 1) break;

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

	// if (_socket != INVALID_SOCKET) {
	// 	closesocket(_socket);
	// 	_socket = INVALID_SOCKET;
	// }
	closeAndInvalidateSocket(_socket);

	// if (tmpEvent != INVALID_HANDLE_VALUE) {
	// 	CloseHandle(tmpEvent);
	// 	tmpEvent = INVALID_HANDLE_VALUE;
	// }
	closeAndInvalidateHandle(tmpEvent);

	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(m_mutex);
	_live = false;

	write_log(2, "Finished Thead, %s %d %s\n", __FILENAME__, __LINE__, __func__);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN64
bool ConnectClient::recvHandler(HANDLE& hEvent)
#else
bool ConnectClient::recvHandler()
#endif
{
	write_log(2, "クライアント(%ld)からデータを受信, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		write_log(4, "recv error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		write_log(4, "クライアント(%ld)との接続が切れました, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
#ifdef _WIN64
		deleteConnection(hEvent);
#else
		// close(_socket);
		// _socket = INVALID_SOCKET;// ★★★konishi
		closeAndInvalidateSocket(_socket);
#endif
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
#ifdef _WIN64
		deleteConnection(hEvent);
#else
		// close(_socket);
		// _socket = INVALID_SOCKET;// ★★★konishi
		closeAndInvalidateSocket(_socket);
#endif
		return true;
	}

	write_log(2, "変換後:[%s] , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	return true;
}

#ifdef _WIN64
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
// CloseHandleのラッパー
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeAndInvalidateHandle(HANDLE& hEvent)
{
	if (hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(hEvent);
		hEvent = INVALID_HANDLE_VALUE;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::deleteConnection(HANDLE& hEvent)
{
	// if (_socket != INVALID_SOCKET) {
	// 	closesocket(_socket);
	// 	_socket = INVALID_SOCKET;
		closeAndInvalidateSocket(_socket);
	// }

	// if (hEvent != INVALID_HANDLE_VALUE) {
	// 	CloseHandle(hEvent);
	// 	hEvent = INVALID_HANDLE_VALUE;
	// }
	closeAndInvalidateHandle(hEvent);

	return;
}

#endif
///////////////////////////////////////////////////////////////////////////////
// closeのラッパー
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeAndInvalidateSocket(SOCKET& socket)
{
	if (_socket != INVALID_SOCKET) {
#ifdef _WIN64
		closesocket(socket);
#else
		close(socket);
#endif
		socket = INVALID_SOCKET;
	}
	return true;
}