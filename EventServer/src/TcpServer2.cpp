///////////////////////////////////////////////////////////////////////////////
// [EventServer2]
// WinSockを使用したTCPサーバー。
// シングルスレッドで最大32クライアントと同時接続を行う。
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <WinSock2.h>
#include <map>
#include <thread>
///////////////////////////////////////////////////////////////////////////////
// プロトタイプ宣言
///////////////////////////////////////////////////////////////////////////////
bool closeHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent);
bool recvHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent);
bool acceptHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent);
void deleteConnection(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent);

///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
bool main_thread_flag = true;
HANDLE	hMutex; //ミューテックスのハンドル

#define CLIENT_MAX	32					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)

///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	std::map<HANDLE, SOCKET> _socketMap;
	HANDLE _tmpEvent;
public:
	ConnectClient(std::map<HANDLE, SOCKET>& socketMap, HANDLE& tmpEvent) {
		_socketMap = socketMap;
		_tmpEvent = tmpEvent;
	};
	~ConnectClient() {
	};
public:
	void operator()() {
		while (1) {
			WaitForSingleObject(hMutex, INFINITE); //mutex 間は他のスレッドから変数を変更できない
			if (!main_thread_flag) {
				ReleaseMutex(hMutex);
				break;
			}
			ReleaseMutex(hMutex);

			//イベントを配列で管理
			const int workierEventNum = 1;
			HANDLE workerEventList[workierEventNum];
			workerEventList[0] = _tmpEvent;

			printf("書き込みを待っています.\n");
			DWORD worker_dwTimeout = TIMEOUT_MSEC;

			//イベント多重待ち
			int worker_nRet = WSAWaitForMultipleEvents(workierEventNum, workerEventList, FALSE, worker_dwTimeout, FALSE);
			if (worker_nRet == WSA_WAIT_FAILED)
			{
				printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
				break;
			}

			if (worker_nRet == WSA_WAIT_TIMEOUT) {
				printf("タイムアウト発生!!!\n");
				continue;
			}

			printf("WSAWaitForMultipleEvents nRet=%ld\n", worker_nRet);

			// イベントを検知したHANDLE
			HANDLE workerHandle = workerEventList[worker_nRet];

			// イベントを検知したHANDLEと関連付けしているソケット
			SOCKET workerSocket = _socketMap[workerHandle];

			//イベント調査
			WSANETWORKEVENTS events;
			if (WSAEnumNetworkEvents(workerSocket, workerHandle, &events) == SOCKET_ERROR)
			{
				printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
				break;
			}

			//READ
			if (events.lNetworkEvents & FD_READ)
			{
				// クライアントとの通信ソケットにデータが到着した
				recvHandler(_socketMap, workerHandle);
			}

			//CLOSE
			if (events.lNetworkEvents & FD_CLOSE)
			{
				// クライアントとの通信ソケットのクローズを検知
				closeHandler(_socketMap, workerHandle);
			}
		}

		deleteConnection(_socketMap, _tmpEvent);
		printf("Finished Thead\n");
	}
};
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	///////////////////////////////////
	// コマンド引数の解析
	///////////////////////////////////
	if (argc != 2) {
		printf("TcpServer portNo\n");
		return -1;
	}

	HANDLE hPipe;
	hPipe = CreateNamedPipe("\\\\.\\pipe\\EventServer",
							PIPE_ACCESS_INBOUND,
							PIPE_TYPE_MESSAGE | PIPE_WAIT,
							1, 0, 0, 150, (LPSECURITY_ATTRIBUTES)NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	// ポート番号の設定
	int nPortNo;            // ポート番号
	nPortNo = atol(argv[1]);

	int nRet = 0;

	// WINSOCKの初期化(これやらないとWinSock2.hの内容が使えない)
	WSADATA	WsaData;
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup(wVersionRequested, &WsaData) != 0) {
		printf("WSAStartup() error. code=%d\n", WSAGetLastError());
		return -1;
	}

	// ソケットの生成(listen用)
	SOCKET srcSocket;
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (srcSocket == -1) {
		printf("socket error\n");
		return -1;
	}

	HANDLE hEvent = WSACreateEvent();
	if (hEvent == INVALID_HANDLE_VALUE) {
		printf("WSACreateEvent error\n");
		return -1;
	}

	// UnixのSelectのような意味合いではない。該当ソケットはどのイベントにのみ反応するのかを定義する関数。
	// クライアントからの接続待ちソケットなのでFD_ACCEPTのみ関連付ける
	nRet = WSAEventSelect(srcSocket, hEvent, FD_ACCEPT);
	if (nRet == SOCKET_ERROR)
	{
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	///////////////////////////////////
	// socketの設定
	///////////////////////////////////
	// listen用sockaddrの設定
	struct sockaddr_in srcAddr;
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(nPortNo);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = INADDR_ANY;

	// ソケットのバインド
	nRet = bind(srcSocket,(struct sockaddr*)&srcAddr,sizeof(srcAddr));
	if (nRet == SOCKET_ERROR) {
		printf("bind error. (%ld)\n", WSAGetLastError());
		return -1;
	}

	// クライアントからの接続待ち
	nRet = listen(srcSocket, 1);
	if (nRet == SOCKET_ERROR) {
		printf("listen error. (%ld)\n", WSAGetLastError());
		return -1;
	}

	std::map<HANDLE, SOCKET> socketMap;
	socketMap[hEvent] = srcSocket;//listenソケットとイベントを結び付ける

	hMutex = CreateMutex(NULL, FALSE, NULL);	//ミューテックス生成

	while (main_thread_flag) {
		//イベントを配列で管理
		const int mainEventNum = 1;
		HANDLE eventList[mainEventNum];
		eventList[0] = hEvent;//listenイベントのみ

		printf("新規接続を待っています.\n");
		//DWORD dwTimeout = WSA_INFINITE;	// 無限待ち
		DWORD dwTimeout = TIMEOUT_MSEC;

		nRet = WSAWaitForMultipleEvents(mainEventNum, eventList, FALSE, dwTimeout, FALSE);
		if (nRet == WSA_WAIT_FAILED)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			break;
		}

		if (nRet == WSA_WAIT_TIMEOUT) {
			printf("タイムアウト発生!!!\n");
			continue;
		}

		printf("WSAWaitForMultipleEvents nRet=%ld\n", nRet);

		// イベントを検知したHANDLE
		HANDLE mainHandle = eventList[nRet];//これはhEventの事

		// クライアントから新規接続を検知
		acceptHandler(socketMap, mainHandle);
	}

	// ソケットとイベントHANDLEをクローズ
	for (std::map<HANDLE, SOCKET>::iterator ite = socketMap.begin(); ite != socketMap.end(); ++ite)
	{
		CloseHandle(ite->first);
		closesocket(ite->second);
	}
	socketMap.clear();

	WSACleanup();

	printf("正常終了します\n");
	return(0);
}

///////////////////////////////////////////////////////////////////////////////
// クライアントから新規接続受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool acceptHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	printf("クライアント接続要求を受け付けました\n");
	int	addrlen;
	struct sockaddr_in dstAddr;
	addrlen = sizeof(dstAddr);

	SOCKET sock = socketMap[hEvent];

	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		printf("accept error. (%ld)\n", WSAGetLastError());
		return true;
	}

	printf("[%s]から接続を受けました. newSock=%d\n", inet_ntoa(dstAddr.sin_addr), newSock);

	if (socketMap.size() == CLIENT_MAX) {
		printf("同時接続可能クライアント数を超過\n");
		closesocket(newSock);
		return false;
	}

	HANDLE tmpEvent = WSACreateEvent();

	//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
	int nRet = WSAEventSelect(newSock, tmpEvent, FD_READ | FD_CLOSE);
	if (nRet == SOCKET_ERROR) {
		printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
		return false;
	}
	
	socketMap[tmpEvent] = newSock;

	//Client対応専用スレッドへ
	std::thread th{ ConnectClient(socketMap, tmpEvent) };
	th.detach();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool recvHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	SOCKET sock = socketMap[hEvent];
	printf("クライアント(%d)からデータを受信\n", sock);

	char buf[1024];
	int stSize = recv(sock, buf, sizeof(buf), 0);
	if (stSize <= 0) {
		printf("recv error.\n");
		printf("クライアント(%d)との接続が切れました\n", sock);
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
bool closeHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	SOCKET sock = socketMap[hEvent];

	printf("クライアント(%d)との接続が切れました\n", sock);
	deleteConnection(socketMap, hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ、mapからの削除
///////////////////////////////////////////////////////////////////////////////
void deleteConnection(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	SOCKET sock = socketMap[hEvent];
	closesocket(sock);
	CloseHandle(hEvent);
	socketMap.erase(hEvent);
	return;
}

