TARGET1 = ServerMain
TARGET2 = TcpClient
TARGET3 = Stop
TARGET4 = Start
CCFLAG = -std=c++11 -g -DBOOST_LOG_DYN_LINK

all : $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

$(TARGET1) : ServerMain.o TcpServer.o BoostLog.o TcpCommon.o ConnectClient.o 
# -DBOOST_LOG_DYN_LINK　をここに書いても意味がないのでCCFLAGに引っ越し
	g++ -o $(TARGET1) ServerMain.o TcpServer.o BoostLog.o TcpCommon.o ConnectClient.o -lpthread -lboost_log -lboost_thread -lboost_system
	
$(TARGET2) : TcpClient.o 
	g++ -o $(TARGET2) TcpClient.o -lrt

$(TARGET3) : Stop.o 
	g++ -o $(TARGET3) Stop.o -lrt

$(TARGET4) : Start.o TcpCommon.o BoostLog.o
	g++ -o $(TARGET4) Start.o TcpCommon.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_system

ServerMain.o : ServerMain.cpp BoostLog.h thread_pool.h TcpCommon.h ConnectClient.h
	g++ -c $(CCFLAG) ServerMain.cpp

TcpServer.o : TcpServer.cpp
	g++ -c $(CCFLAG) TcpServer.cpp

TcpClient.o : TcpClient.cpp
	g++ -c $(CCFLAG) TcpClient.cpp

Stop.o : Stop.cpp
	g++ -c $(CCFLAG) Stop.cpp

BoostLog.o : BoostLog.cpp BoostLog.h
	g++ -c $(CCFLAG) BoostLog.cpp

Start.o : Start.cpp TcpCommon.h
	g++ -c $(CCFLAG) Start.cpp

TcpCommon.o : TcpCommon.cpp TcpCommon.h BoostLog.h
	g++ -c $(CCFLAG) TcpCommon.cpp

ConnectClient.o : ConnectClient.cpp ConnectClient.h
	g++ -c $(CCFLAG) ConnectClient.cpp
clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) *.o

