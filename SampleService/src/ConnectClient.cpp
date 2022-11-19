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
	writeLog(2, "destructor started. socket:%d, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
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
	writeLog(2, "constructor started. socket:%d, %s %d %s\n", dstSocket, __FILENAME__, __LINE__, __func__);
	_socket = dstSocket;
	live = true;
}


///////////////////////////////////////////////////////////////////////////////
//ハンドラ制御
///////////////////////////////////////////////////////////////////////////////
void ConnectClient::func()
{
#ifndef _WIN64
	writeLog(2, "func started. socket:%d, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);

	while (1) {
		//終了確認
		if (getServerStatus() == 1) {
			writeLog(2, "Finish thread:%s, %s %d %s\n", std::this_thread::get_id(), __FILENAME__, __LINE__, __func__);
			break;
		}

		///////////////////////////////////
		// 通信
		///////////////////////////////////
		writeLog(2, "Start connection with client. socket:%d, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
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
				//writeLog(4, "select error. errno=%d, %s %d %s\n", errno, __FILENAME__, __LINE__, __func__);
				writeLog(4, "%ld:%s, %s %d %s\n", errno, "select error.\n", __FILENAME__, __LINE__, __func__);
				break;
			}
		}
		else if (nRet == 0) {
			writeLog(2, "workerスレッドでタイムアウト発生, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		// クライアントとの通信ソケットにデータが到着した
		recvHandler();
	}
	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(mMutex);
	live = false;
#else
	HANDLE tmpEvent = WSACreateEvent();

	//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
	int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		//writeLog(5, "WSAEventSelect error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
		writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAEventSelect error.\n", __FILENAME__, __LINE__, __func__);
		
		return;
	}

	//イベントを配列で管理
	const int workierEventNum = 1;
	HANDLE workerEventList[workierEventNum];
	workerEventList[0] = tmpEvent;

	while (1) {
		//サーバーステータスチェック
		if (getServerStatus() == 1) break;

		//強制終了用のinterruption_pointを張る
		boost::this_thread::interruption_point();

		writeLog(2, "Waiting for input.,%s %d %s\n", __FILENAME__, __LINE__, __func__);
		DWORD worker_dwTimeout = TIMEOUT_MSEC;

		//イベント多重待ち
		int worker_nRet = WSAWaitForMultipleEvents(workierEventNum,
			workerEventList,
			FALSE,
			worker_dwTimeout,
			FALSE);
		if (worker_nRet == WSA_WAIT_FAILED)
		{
			//writeLog(5, "WSAWaitForMultipleEvents wait error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAWaitForMultipleEvents wait error.\n", __FILENAME__, __LINE__, __func__);
			break;
		}

		if (worker_nRet == WSA_WAIT_TIMEOUT) {
			writeLog(2, "タイムアウト発生です!!!, %s %d %s\n", __FILENAME__, __LINE__, __func__);
			continue;
		}

		writeLog(2, "WSAWaitForMultipleEvents nRet=%ld, %s %d %s\n", worker_nRet, __FILENAME__, __LINE__, __func__);

		// イベントを検知したHANDLE
		HANDLE workerHandle = workerEventList[worker_nRet];

		//イベント調査
		WSANETWORKEVENTS events;
		if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
		{
			//writeLog(5, "WSAWaitForMultipleEvents error. (%ld), %s %d %s\n", WSAGetLastError(), __FILENAME__, __LINE__, __func__);
			writeLog(5, "%ld:%s, %s %d %s\n", WSAGetLastError(), "WSAWaitForMultipleEvents error.\n", __FILENAME__, __LINE__, __func__);
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
//			if (workerHandle == INVALID_HANDLE_VALUE)//if copy is invalid
//			{
//				tmpEvent = INVALID_HANDLE_VALUE;//invalidate original
//			}
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
	//closeAndInvalidateHandle(tmpEvent);

	//while抜けたらフラグ倒す
	std::lock_guard<std::mutex> lk(mMutex);
	live = false;

	writeLog(2, "Finished Thead, %s %d %s\n", __FILENAME__, __LINE__, __func__);
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
	char messageBuf[1024];

	writeLog(2, "クライアント(%ld)からデータを受信, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
	char buf[1024];
	int stSize = recv(_socket, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		//writeLog(4, "recv error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		//writeLog(4, "クライアント(%ld)との接続が切れました, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
		sprintf(messageBuf, "recv error.\nクライアント(%ld)との接続が切れました", _socket);
#ifdef _WIN64
		writeLog(4, "%ld:%s, %s %d %s\n", WSAGetLastError(), messageBuf, __FILENAME__, __LINE__, __func__);
#else
		writeLog(4, "%ld:%s, %s %d %s\n", errno, messageBuf, __FILENAME__, __LINE__, __func__);
#endif

#ifdef _WIN64
		deleteConnection(hEvent);
#else
		// close(_socket);
		// _socket = INVALID_SOCKET;// ★★★konishi
		closeAndInvalidateSocket(_socket);
#endif
		return true;
	}

	writeLog(2, "変換前:[%s] %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);

	for (int i = 0; i < (int)stSize; i++) { // bufの中の小文字を大文字に変換
		if (isalpha(buf[i])) {
			buf[i] = toupper(buf[i]);
		}
	}

	// クライアントに返信
	stSize = send(_socket, buf, strlen(buf) + 1, 0);

	if (stSize != strlen(buf) + 1) {
		//writeLog(4, "send error., %s %d %s\n", __FILENAME__, __LINE__, __func__);
		//writeLog(4, "クライアントとの接続が切れました, %s %d %s\n", __FILENAME__, __LINE__, __func__);
		sprintf(messageBuf, "send error.\nクライアント(%ld)との接続が切れました", _socket);
#ifdef _WIN64
		writeLog(4, "%ld:%s, %s %d %s\n", WSAGetLastError(), messageBuf, __FILENAME__, __LINE__, __func__);
#else
		writeLog(4, "%ld:%s, %s %d %s\n", errno, messageBuf, __FILENAME__, __LINE__, __func__);
#endif

#ifdef _WIN64
		deleteConnection(hEvent);
#else
		// close(_socket);
		// _socket = INVALID_SOCKET;
		closeAndInvalidateSocket(_socket);
#endif
		return true;
	}

	writeLog(2, "変換後:[%s] , %s %d %s\n", buf, __FILENAME__, __LINE__, __func__);
	return true;
}

#ifdef _WIN64
///////////////////////////////////////////////////////////////////////////////
// クライアントとの通信ソケットの切断検知時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool ConnectClient::closeHandler(HANDLE& hEvent)
{
	char messageBuf[1024];
	//writeLog(4, "クライアント(%d)との接続が切れました, %s %d %s\n", _socket, __FILENAME__, __LINE__, __func__);
	sprintf(messageBuf, "send error.\nクライアント(%ld)との接続が切れました", _socket);
#ifdef _WIN64
	writeLog(4, "%ld:%s, %s %d %s\n", WSAGetLastError(), messageBuf, __FILENAME__, __LINE__, __func__);
#else
	writeLog(4, "%ld:%s, %s %d %s\n", errno, messageBuf, __FILENAME__, __LINE__, __func__);
#endif
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