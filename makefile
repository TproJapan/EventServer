TARGET1 = TcpServer
TARGET2 = TcpClient
TARGET3 = Stop
CCFLAG = -std=c++11 -g

all : $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1) : TcpServer.o BoostLog.o thread_pool.o
#	g++ -o $(TARGET1) TcpServer.o BoostLog.o -lpthread
	g++ -DBOOST_LOG_DYN_LINK -o $(TARGET1) TcpServer.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_system

$(TARGET2) : TcpClient.o 
	g++ -o $(TARGET2) TcpClient.o -lrt

$(TARGET3) : Stop.o 
	g++ -o $(TARGET3) Stop.o -lrt

TcpServer.o : TcpServer.cpp BoostLog.h
	g++ -c $(CCFLAG) TcpServer.cpp

TcpClient.o : TcpClient.cpp
	g++ -c $(CCFLAG) TcpClient.cpp

Stop.o : Stop.cpp
	g++ -c $(CCFLAG) Stop.cpp

BoostLog.o : BoostLog.cpp BoostLog.h
	g++ -c $(CCFLAG) BoostLog.cpp

thread_pool.o : thread_pool.cpp thread_pool.h
	g++ -c $(CCFLAG) thread_pool.cpp

clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) *.o

