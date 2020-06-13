TARGET1 = TcpServer
TARGET2 = TcpClient
TARGET3 = Stop
CCFLAG = -std=c++11 -g

all : $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1) : TcpServer.o
	g++ -o $(TARGET1) TcpServer.o -lpthread

$(TARGET2) : TcpClient.o 
	g++ -o $(TARGET2) TcpClient.o -lrt

$(TARGET3) : Stop.o 
	g++ -o $(TARGET3) Stop.o -lrt

TcpServer.o : TcpServer.cpp
	g++ -c $(CCFLAG) TcpServer.cpp

TcpClient.o : TcpClient.cpp
	g++ -c $(CCFLAG) TcpClient.cpp

Stop.o : Stop.cpp
	g++ -c $(CCFLAG) Stop.cpp

clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) *.o

