TARGET1 = TcpServer
TARGET2 = TcpClient
TARGET3 = Stop
CCFLAG = -std=c++11 -g

all : $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1) : TcpServer.o BoostLog.o
#	g++ -o $(TARGET1) TcpServer.o BoostLog.o -lpthread
	g++ -DBOOST_LOG_DYN_LINK -o $(TARGET1) TcpServer.o BoostLog.o -lpthread -lboost_log -lboost_thread -lboost_log_setup -lboost_system
#	g++ -DBOOST_LOG_DYN_LINK -o $(TARGET1) TcpServer.o BoostLog.o -lpthread -lboost_log \
#	 -lboost_thread -lboost_system -lboost_log_setup -lboost_atomic -lboost_chrono \
#	 -lboost_container -lboost_context -lboost_coroutine -lboost_date_time -lboost_exception \
#	 -lboost_fiber -lboost_filesystem -lboost_graph -lboost_graph_parallel -lboost_iostreams \
#	 -lboost_locale -lboost_log -lboost_log_setup -lboost_math_c99 -lboost_math_c99f \
#	 -lboost_math_c99l -lboost_math_tr1 -lboost_math_tr1f -lboost_math_tr1l -lboost_mpi \
#	 -lboost_mpi_python38 -lboost_numpy38 -lboost_prg_exec_monitor -lboost_program_options \
#	 -lboost_python38 -lboost_random -lboost_regex -lboost_serialization \
#	 -lboost_stacktrace_addr2line -lboost_stacktrace_backtrace -lboost_stacktrace_basic \
#	 -lboost_stacktrace_noop -lboost_system -lboost_test_exec_monitor -lboost_thread \
#	 -lboost_timer -lboost_type_erasure -lboost_unit_test_framework -lboost_wave \
#	 -lboost_wserialization


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

clean :
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) *.o

