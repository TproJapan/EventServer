TARGET1 = TcpServer
TARGET2 = TcpClient
TARGET3 = Stop
TARGET4 = Start
#CCFLAG = -std=c++11 -g
CCFLAG = -std=c++11 -g -DBOOST_LOG_DYN_LINK

all : $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

#$(TARGET1) : TcpServer.o BoostLog.o thread_pool.o
$(TARGET1) : TcpServer.o BoostLog.o TcpCommon.o
#	g++ -o $(TARGET1) TcpServer.o BoostLog.o -lpthread
#	g++ -DBOOST_LOG_DYN_LINK -o $(TARGET1) TcpServer.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_system
# -DBOOST_LOG_DYN_LINK　をここに書いても意味がないのでCCFLAGに引っ越し
	g++ -o $(TARGET1) TcpServer.o BoostLog.o TcpCommon.o -lpthread -lboost_log -lboost_thread -lboost_system
	
$(TARGET2) : TcpClient.o 
	g++ -o $(TARGET2) TcpClient.o -lrt

$(TARGET3) : Stop.o 
	g++ -o $(TARGET3) Stop.o -lrt

$(TARGET4) : Start.o TcpCommon.o semlock.o BoostLog.o
	g++ -o $(TARGET4) Start.o TcpCommon.o semlock.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_system

#TcpServer.o : TcpServer.cpp BoostLog.h
TcpServer.o : TcpServer.cpp BoostLog.h thread_pool.h TcpCommon.h ConnectClient.h
	g++ -c $(CCFLAG) TcpServer.cpp

TcpClient.o : TcpClient.cpp
	g++ -c $(CCFLAG) TcpClient.cpp

Stop.o : Stop.cpp
	g++ -c $(CCFLAG) Stop.cpp

BoostLog.o : BoostLog.cpp BoostLog.h
	g++ -c $(CCFLAG) BoostLog.cpp

Start.o : Start.cpp semlock.h TcpCommon.h
	g++ -c $(CCFLAG) Start.cpp

semlock.o : semlock.cpp semlock.h BoostLog.h
	g++ -c $(CCFLAG) semlock.cpp

TcpCommon.o : TcpCommon.cpp TcpCommon.h BoostLog.h
	g++ -c $(CCFLAG) TcpCommon.cpp

clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) *.o

