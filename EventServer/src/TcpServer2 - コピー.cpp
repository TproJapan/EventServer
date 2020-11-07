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
int checkServerStatus();
///////////////////////////////////////////////////////////////////////////////
// 共用変数
///////////////////////////////////////////////////////////////////////////////
int server_status = 0;//サーバーステータス(0:起動, 1:シャットダウン)
//bool main_thread_flag = true;
std::map<HANDLE, SOCKET> socketMap;
HANDLE	server_status_Mutex;
HANDLE socketMap_Mutex;
const char* PIPE_NAME = "\\\\%s\\pipe\\EventServer";
#define CLIENT_MAX	32					// 同時接続可能クライアント数
#define TIMEOUT_MSEC	3000			// タイムアウト時間(ミリ秒)

///////////////////////////////////////////////////////////////////////////////
//引数あり、クラスでの関数オブジェクト
///////////////////////////////////////////////////////////////////////////////
class ConnectClient {
	SOCKET _socket;
public:
	ConnectClient(SOCKET& tmpsocket) {
		_socket = tmpsocket;
	};
	~ConnectClient() {
	};
public:
	void operator()() {
		HANDLE tmpEvent = WSACreateEvent();

		//socketとイベント変数を、どの観点のイベントで反応させるかを紐づけ
		int nRet = WSAEventSelect(_socket, tmpEvent, FD_READ | FD_CLOSE);
		if (nRet == SOCKET_ERROR) {
			printf("WSAEventSelect error. (%ld)\n", WSAGetLastError());
			return;
		}

		WaitForSingleObject(socketMap_Mutex, INFINITE); //mutex間は他のスレッドから変数を変更できない
		socketMap[tmpEvent] = _socket;
		ReleaseMutex(socketMap_Mutex);

		//イベントを配列で管理
		const int workierEventNum = 1;
		HANDLE workerEventList[workierEventNum];
		workerEventList[0] = tmpEvent;

		while (1) {
			//サーバーステータスチェック
			if (checkServerStatus() == 1) break;

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
				recvHandler(socketMap, workerHandle);
			}

			//CLOSE
			if (events.lNetworkEvents & FD_CLOSE)
			{
				// クライアントとの通信ソケットのクローズを検知
				closeHandler(socketMap, workerHandle);
			}
		}

		deleteConnection(socketMap, tmpEvent);
		printf("Finished Thead\n");
	}
};
///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	int nRet = 0;

	///////////////////////////////////
	// コマンド引数の解析
	///////////////////////////////////
	if (argc != 2) {
		printf("TcpServer portNo\n");
		return -1;
	}

	// パイプ名の組み立て
	char pipeName[80];
	wsprintf(pipeName, PIPE_NAME, ".");

	// 名前付きパイプの作成
	HANDLE hPipe;
	hPipe = CreateNamedPipe(pipeName,		// パイプ名
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,		// パイプのアクセスモード
		PIPE_TYPE_MESSAGE,		// パイプの種類, 待機モード
		1,		// インスタンスの最大数
		0,		// 出力バッファのサイズ
		0,		// 入力バッファのサイズ
		150,	// タイムアウト値
		(LPSECURITY_ATTRIBUTES)NULL);	// セキュリティ属性
	if (hPipe == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	// OVARLAPPED構造体の初期設定
	OVERLAPPED overlappedConnect;
	memset(&overlappedConnect, 0, sizeof(overlappedConnect));
	HANDLE eventConnect = CreateEvent(0, FALSE, FALSE, 0);
	if (eventConnect == INVALID_HANDLE_VALUE) {
		printf("CreateNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}
	overlappedConnect.hEvent = eventConnect;

	// パイプクライアントからの接続を待つ。
	// ただしOVERLAP指定なのでクライアントからの接続が無くても正常終了する
	// (実際の接続確認はWSAWaitForMultipleEvents, GetOverlappedResultで行う)
	BOOL bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
	if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING) {	// konishi
		printf("ConnectNamedPipe error. (%ld)\n", GetLastError());
		return -1;
	}

	// ポート番号の設定
	int nPortNo;            // ポート番号
	nPortNo = atol(argv[1]);

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

	socketMap_Mutex = CreateMutex(NULL, FALSE, NULL);
	if (socketMap_Mutex == NULL) {
		printf("CreateMutex error. (%ld)\n", GetLastError());
		return -1;
	}
	WaitForSingleObject(socketMap_Mutex, INFINITE);
	socketMap[hEvent] = srcSocket;//listenソケットとイベントを結び付ける
	socketMap[eventConnect] = NULL;	//名前付きパイプに対する接続待ちハンドルには対応ソケットは無いのでNULL
	ReleaseMutex(socketMap_Mutex);

	server_status_Mutex = CreateMutex(NULL, FALSE, NULL);	//ミューテックス生成

	while (1) {
		if (checkServerStatus() == 1) break;

		//イベントを配列で管理
		const int mainEventNum = 2;
		HANDLE eventList[mainEventNum];
		eventList[0] = hEvent;//listenイベントのみ
		eventList[1] = eventConnect;//名前付きパイプ

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
		HANDLE mainHandle = eventList[nRet];

		//ハンドル書き込み検出
		if (mainHandle == eventConnect) {
			printf("パイプに接続要求を受けました\n");

			DWORD byteTransfer;
			DWORD NumBytesRead;
			DWORD dwRet = 0;
			BOOL bRet;

			bRet = GetOverlappedResult(mainHandle, &overlappedConnect, &byteTransfer, TRUE);
			printf("GetOverlappedResult bRet = %ld\n", bRet);
			if (bRet != TRUE) {
				printf("GetOverlappedResult error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}
			ResetEvent(mainHandle);

			// パイプクライアントからのメッセージを受信
			char buf[1024];
			bRet = ReadFile(hPipe,
				buf,
				sizeof(buf),
				&NumBytesRead,
				(LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				printf("ReadFile error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}

			printf("クライアントと接続しました:[%s]\n", buf);

			// 受信メッセージが "stop" の場合はTcpServerを停止
			if (strcmp(buf, "stop") == 0) {
				WaitForSingleObject(server_status_Mutex, INFINITE);
				server_status = 1;
				ReleaseMutex(server_status_Mutex);
				printf("サーバ停止要求を受信しました\n");
			}

			// パイプクライアントに応答メッセージを送信
			DWORD NumBytesWritten;
			strcpy(buf, "OK");
			bRet = WriteFile(hPipe, buf, (DWORD)strlen(buf) + 1,
				&NumBytesWritten, (LPOVERLAPPED)NULL);
			if (bRet != TRUE) {
				printf("WriteFile error\n");
				//DisconnectNamedPipe(hPipe);
				break;
			}

			// クライアントとのパイプを切断
			DisconnectNamedPipe(hPipe);

			// 新たなパイプクライアントからの接続を待つ
			bRet = ConnectNamedPipe(hPipe, &overlappedConnect);
			if (bRet != TRUE) {
				printf("ConnectNamedPipe error. (%ld)\n", GetLastError());
				if (GetLastError() != ERROR_IO_PENDING) break;
			}

			continue;
		}

		//イベント調査&イベントハンドルのリセット。これを発行しないとイベントリセットされないので常にイベントが発生している事になる
		WSANETWORKEVENTS mainEvents;
		if (WSAEnumNetworkEvents(srcSocket, mainHandle, &mainEvents) == SOCKET_ERROR)
		{
			printf("WSAWaitForMultipleEvents error. (%ld)\n", WSAGetLastError());
			break;
		}

		//Acceptフラグを見ておく
		if (mainEvents.lNetworkEvents & FD_ACCEPT)
		{
			// クライアントから新規接続を検知
			acceptHandler(socketMap, mainHandle);
		}
	}

	//パイプをクローズ
	DisconnectNamedPipe(hPipe);

	// ソケットとイベントHANDLEをクローズ
	WaitForSingleObject(socketMap_Mutex, INFINITE);
	for (std::map<HANDLE, SOCKET>::iterator ite = socketMap.begin(); ite != socketMap.end(); ++ite)
	{
		CloseHandle(ite->first);
		closesocket(ite->second);
	}
	socketMap.clear();
	ReleaseMutex(socketMap_Mutex);

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

	WaitForSingleObject(socketMap_Mutex, INFINITE);
	SOCKET sock = socketMap[hEvent];
	ReleaseMutex(socketMap_Mutex);

	SOCKET newSock = accept(sock, (struct sockaddr*)&dstAddr, &addrlen);
	if (newSock == INVALID_SOCKET)
	{
		printf("accept error. (%ld)\n", WSAGetLastError());
		return true;
	}

	printf("[%s]から接続を受けました. newSock=%ld\n", inet_ntoa(dstAddr.sin_addr), newSock);

	WaitForSingleObject(socketMap_Mutex, INFINITE);
	int socket_size = socketMap.size();
	ReleaseMutex(socketMap_Mutex);
	if (socket_size == CLIENT_MAX) {
		printf("同時接続可能クライアント数を超過\n");
		closesocket(newSock);
		return false;
	}

	//Client対応専用スレッドへ
	std::thread th{ ConnectClient(newSock) };
	th.detach();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// クライアントからのデータ受付時のハンドラ
///////////////////////////////////////////////////////////////////////////////
bool recvHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	WaitForSingleObject(socketMap_Mutex, INFINITE);
	SOCKET sock = socketMap[hEvent];
	ReleaseMutex(socketMap_Mutex);
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
bool closeHandler(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	WaitForSingleObject(socketMap_Mutex, INFINITE);
	SOCKET sock = socketMap[hEvent];
	ReleaseMutex(socketMap_Mutex);

	printf("クライアント(%d)との接続が切れました\n", sock);
	deleteConnection(socketMap, hEvent);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// 指定されたイベントハンドルとソケットクローズ、mapからの削除
///////////////////////////////////////////////////////////////////////////////
void deleteConnection(std::map<HANDLE, SOCKET>& socketMap, HANDLE& hEvent)
{
	WaitForSingleObject(socketMap_Mutex, INFINITE);
	SOCKET sock = socketMap[hEvent];
	closesocket(sock);
	CloseHandle(hEvent);
	socketMap.erase(hEvent);
	ReleaseMutex(socketMap_Mutex);
	return;
}

int checkServerStatus() {
	WaitForSingleObject(server_status_Mutex, INFINITE); //mutex間は他のスレッドから変数を変更できない
	int status = server_status;
	ReleaseMutex(server_status_Mutex);
	return status;
}

