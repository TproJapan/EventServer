#pragma once
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <map>
#include <thread>
#include "CSocketMap.h"
#include "thread_pool.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "CommonVariables.h"
#include "CommonFunc.h"

///////////////////////////////////////////////////////////////////////////////
//クライアント接続クラス
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
	CSocketMap* _pSocketMap;
public:
	ConnectClient(SOCKET& tmpsocket, CSocketMap* tmpSocketMapPtr) {
		_socket = tmpsocket;
		_pSocketMap = tmpSocketMapPtr;
	};
	~ConnectClient() {
	};

	///////////////////////////////////////////////////////////////////////////////
	//ハンドラ制御
	///////////////////////////////////////////////////////////////////////////////
public:
	void func() {
		HANDLE tmpEvent = WSACreateEvent();

		//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
		int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
		if (nRet == SOCKET_ERROR) {
			printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
			return;
		}

		pSocketMap->addSocket(_socket, tmpEvent);

		//イベントを配列で管理
		const int workierEventNum = 1;
		HANDLE workerEventList[workierEventNum];
		workerEventList[0] = tmpEvent;

		while (1) {
			//サーバーステータスチェック
			if (this->checkServerStatus() == 1) break;

			printf("書き込みを待っています.\n");
			DWORD worker_dwTimeout = TIMEOUT_MSEC;

			//イベント多重待ち
			int worker_nRet = WSAWaitForMultipleEvents(workierEventNum, workerEventList, FALSE, worker_dwTimeout, FALSE);
			if (worker_nRet == WSA_WAIT_FAILED)
			{
				printf("WSAWaitForMultipleEvents wait error. (%ld)\n", WSAGetLastError());
				break;
			}

			if (worker_nRet == WSA_WAIT_TIMEOUT) {
				printf("タイムアウト発生!!!\n");
				continue;
			}

			printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);

			// イベントを検知したHANDLE
			HANDLE workerHandle = workerEventList[worker_nRet];

			//イベント調査
			WSANETWORKEVENTS events;
			if (WSAEnumNetworkEvents(_socket, workerHandle, &events) == SOCKET_ERROR)
			{
				printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
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
		printf("Finished Thead\n");
	}

	///////////////////////////////////////////////////////////////////////////////
	// クライアントからのデータ受付時のハンドラ
	///////////////////////////////////////////////////////////////////////////////
private:
	bool recvHandler(CSocketMap& socketMap, HANDLE& hEvent)
	{
		SOCKET sock = socketMap.getSocket(hEvent);
		printf("クライアント(%ld)からデータを受信\n", sock);

		char buf[1024];
		int stSize = recv(sock, buf, sizeof(buf), 0);
		if (stSize <= 0) {
			printf("recv error.\n");
			printf("クライアント(%ld)との接続が切れました\n", sock);
			deleteConnection(socketMap, hEvent);
			return true;
		}

		printf("変換前:[%s] ==> ", buf);
		for (int i = 0; i < (int)stSize; i++) { // bufの中の小文字を大文字に変換
			if (isalpha(buf[i])) {
				buf[i] = toupper(buf[i]);
			}
		}

		// クライアントに返信
		stSize = send(sock, buf, strlen(buf) + 1, 0);
		if (stSize != strlen(buf) + 1) {
			printf("send error.\n");
			printf("クライアントとの接続が切れました\n");
			deleteConnection(socketMap, hEvent);
			return true;
		}

		printf("変換後:[%s] \n", buf);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	// クライアントとの通信ソケットの切断検知時のハンドラ
	///////////////////////////////////////////////////////////////////////////////
private:
	bool closeHandler(CSocketMap& socketMap, HANDLE& hEvent)
	{
		SOCKET sock = socketMap.getSocket(hEvent);

		printf("クライアント(%d)との接続が切れました\n", sock);
		deleteConnection(socketMap, hEvent);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 指定されたイベントハンドルとソケットクローズ、mapからの削除
	///////////////////////////////////////////////////////////////////////////////
private:
	void deleteConnection(CSocketMap& socketMap, HANDLE& hEvent)
	{
		socketMap.deleteSocket(hEvent);
		return;
	}
private:
	int checkServerStatus() {
		WaitForSingleObject(server_status_Mutex, INFINITE); //mutex間は他のスレッドから変数を変更できない
		int status = server_status;
		ReleaseMutex(server_status_Mutex);
		return status;
	}
};