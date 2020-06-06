TARGET1 = TcpServer
TARGET2 = TcpClient
CCFLAG = -std=c++11 -g

all : $(TARGET1) $(TARGET2)

$(TARGET1) : TcpServer.o
	g++ -o $(TARGET1) TcpServer.o -lrt

$(TARGET2) : TcpClient.o 
	g++ -o $(TARGET2) TcpClient.o -lrt

TcpServer.o : TcpServer.cpp
	g++ -c $(CCFLAG) TcpServer.cpp

TcpClient.o : TcpClient.cpp
	g++ -c $(CCFLAG) TcpClient.cpp

clean :
	rm -f $(TARGET1) $(TARGET2) *.o

