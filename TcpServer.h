#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/select.h>
#include <iostream>
#include <sys/wait.h>// wait
#include <err.h>	// err
#include <stdlib.h>	// exit
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <pthread.h>
#include "BoostLog.h"
#include "TcpCommon.h"
#include "ConnectClient.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "thread_pool.h"

typedef std::vector<ConnectClient*> connectclient_vector;

class TcpServer {
public:
    int nPortNo;
    boost::asio::io_service io_service;
    thread_pool tp;
    struct sockaddr_in srcAddr;
    int srcSocket;
public:
	TcpServer(int portNo);
	~TcpServer();
	int Func();
    // ソケット切断済みのconnect_clientの解放処理
    int cleanupConnectClientVec(connectclient_vector& vec);
};