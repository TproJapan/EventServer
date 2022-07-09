CC = g++
BUILDDIR = ../build
SRCDIR    = ./
SOURCES   = $(wildcard $(SRCDIR)/*.cpp)
SOURCES1   = $(SRCDIR)/ServerMain.cpp \
				$(SRCDIR)/TcpServer.cpp \
				$(SRCDIR)/BoostLog.cpp \
				$(SRCDIR)/TcpCommon.cpp \
				$(SRCDIR)/ConnectClient.cpp
				
OBJECTS1   = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES1:.cpp=.o)))
OBJDIR    = $(BUILDDIR)
OBJECTS   = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.cpp=.o)))
#TARGET1 =ServerMain
TARGET1 = $(BUILDDIR)/ServerMain
TARGET2 = TcpClient
TARGET3 = Stop
TARGET4 = Start
#CCFLAG = -std=c++11 -g -DBOOST_LOG_DYN_LINK
CCFLAG = -std=c++11 -g -DBOOST_LOG_DYN_LINK -MMD -MP -Wall#(-Wallは取ると警告レベルが下がる)
LDFLAGS =  -lpthread -lboost_log -lboost_thread -lboost_system
 
all : $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)
#$(TARGET1) : ServerMain.o TcpServer.o BoostLog.o TcpCommon.o ConnectClient.o 
#   $(CC) -o $(TARGET1) ServerMain.o TcpServer.o BoostLog.o TcpCommon.o ConnectClient.o -lpthread -lboost_log -lboost_thread -lboost_system
$(TARGET1) : $(OBJECTS1)
	mkdir -p $(BUILDDIR)
	$(CC) -o $@  $^ $(LDFLAGS)
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR)
	$(CC) $(CCFLAG) $(INCLUDE) -o $@ -c $<
$(TARGET2) : TcpClient.o 
	$(CC) -o $(TARGET2) TcpClient.o -lrt
$(TARGET3) : Stop.o 
	$(CC) -o $(TARGET3) Stop.o -lrt
$(TARGET4) : Start.o TcpCommon.o BoostLog.o
	$(CC) -o $(TARGET4) Start.o TcpCommon.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_system
ServerMain.o : ServerMain.cpp BoostLog.h thread_pool.h TcpCommon.h ConnectClient.h
	$(CC) -c $(CCFLAG) ServerMain.cpp
TcpServer.o : TcpServer.cpp
	$(CC) -c $(CCFLAG) TcpServer.cpp
TcpClient.o : TcpClient.cpp
	$(CC) -c $(CCFLAG) TcpClient.cpp
Stop.o : Stop.cpp
	$(CC) -c $(CCFLAG) Stop.cpp
BoostLog.o : BoostLog.cpp BoostLog.h
	$(CC) -c $(CCFLAG) BoostLog.cpp
Start.o : Start.cpp TcpCommon.h
	$(CC) -c $(CCFLAG) Start.cpp
TcpCommon.o : TcpCommon.cpp TcpCommon.h BoostLog.h
	$(CC) -c $(CCFLAG) TcpCommon.cpp
ConnectClient.o : ConnectClient.cpp ConnectClient.h
	$(CC) -c $(CCFLAG) ConnectClient.cpp
clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) *.o *.d
	rm -f -r $(BUILDDIR)