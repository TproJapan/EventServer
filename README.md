# EventServer
Tcp Server which converts the chars from lower case to upper case, then return to the client.
Using Boost Thread.

# [Install(for Windows)]
(1) Clone the rep

(2) Set Boost Path

    Open the file "EventServer/SampleService/SampleService/Boost_Path.txt"

    Override the first line with your boost include path.

    Override the second line with your boost lib path.

(3) Build the SetBoostPath Solution

    Open the VS Solution file "EventServer/SampleService/SampleService/SetBoostPath/SetBoostPath.sln"

    Build the solution

    Launch the file "EventServer/SampleService/SampleService/SetBoostPath/x64/Debug/SetBoostPath.exe"

(4) Build the SampleService Solution

    Open the VS Solution file "EventServer/SampleService/SampleService/SampleService.sln", 
    and change the "SERVICE_EXE_FILEPATH" in install.cpp and uninstall.cpp with your path.
    Then, build the solution.

(5) Install the "SampleServe" as a Windows Service

    # cd EventServer/SampleService/x64/Debug

    # install.exe

# [Install(for Linux)]
(1) Clone the rep

(2) Make
    Open the file "EventServer/SampleService/src/Boost_Path.txt"

    Override the "PROJ_HOME" in TcpCommon.h with your path.

    Make the project

    # cd EventServer/SampleService/src

    # make


# [Launch(for Windows)]
(1) Start the "SampleServ" Service

(2) Client

    # cd EventServer/SampleService/x64/Debug

    # ipconfig(Get Local IPv4 address)

    # TcpClient.exe IPv4_Address 5000(any valid port num)

# [Stop(for Windows)]

(1) Stop Client

    「T子文字のアルファベットを入力してください」

　　-> Type just ".", then press enter key
  
(2) Stop the "SampleServ" Service

# [Launch(for Linux)]
(1) Start the TCP Server

    # cd EventServer/SampleService/src

    # ./Start 5000(any valid port num)

(2) Start the TCP Client

    # cd EventServer/SampleService/src

    # ifconfig(Get Local IPv4 address)

    # ./TcpClient IPv4_Address 5000(any valid port num)

(3)

# [Stop(for Linux)]
(1) Stop Client

    「T子文字のアルファベットを入力してください」

　　-> Type just ".", then press enter key 

(2) Stop the Tcp Server

    # cd EventServer/SampleService/src

    # ps aux(Get the PS ID for the Tcp Server)

    # ./Stop PS_ID