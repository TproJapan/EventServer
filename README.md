# EventServer
Tcp Server which converts the chars from lower case to upper case, then return to the client.
Using Boost Thread.

# Boost Requirement

Windows 1_76

Linux 1_71

# [Install(for Windows)]
(1) Clone the rep

(2) Set Boost Path

    Open the file "EventServer/SampleService/SampleService/BoostPath.txt"

    Override the first line with your boost include path.

    Override the second line with your boost lib path.

(3) Build the SetBoostPath Solution

    Open the VS Solution file "EventServer/SampleService/SampleService/SetBoostPath/SetBoostPath.sln"

    Build the solution

    Launch the file "EventServer/SampleService/SampleService/SetBoostPath/x64/Debug/SetBoostPath.exe"

(4) Build the SampleService Project Only (Not SampleService Solution)

    Open the VS Solution file "EventServer/SampleService/SampleService/SampleService.sln", 
    build SampleService project.
    Get the full path to SampleService.exe. The path will usually be "<path_to_your_project_root>/EventServer/SampleService/x64/Debug/SampleService.exe"

(5) Build the rest of the Projects

    Override the "SERVICE_EXE_FILEPATH" in Install.cpp(in Install Project) and Uninstall.cpp(in Uninstall Project) with the full path you got in (4).
    Then, build the SampleService solution.

(6) Install the "SampleServe" as a Windows Service

    Open CMD as Admin
    # cd EventServer/SampleService/x64/Debug

    # Install.exe

(7) Add port number for this Application in services file

    Open services file. this file usually be in "C:\Windows\System32\drivers\etc", and add the new line.

    "eventserver      <port_number>/tcp"

    <port_number> must be the one which is not used in your system yet.(recommend over 5000), and save the file.

    If not no port number specified or cannot recognize, then uses default number 5000.

# [Install(for Linux)]
(1) Clone the rep

(2) Make

    # cd EventServer/SampleService/src

    # make

(3) Add port number for this Application in services file

    Open services file. this file usually be in "/etc", and add the new line.

    "eventserver      <port_number>/tcp"

    <port_number> must be the one which is not used in your system yet.(recommend over 5000), and save the file.

    If not no port number specified or cannot recognize, then uses default number 5000.

# [Launch(for Windows)]
(1) Start the "SampleServ" Service

(2) Client

    # cd EventServer/SampleService/x64/Debug

    Get Local IPv4 address
    # ipconfig

    # TcpClient.exe IPv4_Address Port_Number

# [Stop(for Windows)]

(1) Stop Client

    「T子文字のアルファベットを入力してください」

    Type just ".", then press enter key
  
(2) Stop the "SampleServ" Service

# [Launch(for Linux)]
(1) Start the TCP Server

    # cd EventServer/SampleService/src

    # ./Start
    pid = ***

(2) Start the TCP Client

    # cd EventServer/SampleService/src

    Get Local IPv4 address
    # ifconfig

    # ./TcpClient IPv4_Address Port_Number

# [Stop(for Linux)]
(1) Stop Client

    「T子文字のアルファベットを入力してください」

    Type just ".", then press enter key 

(2) Stop the Tcp Server

    # cd EventServer/SampleService/src

    # ./Stop <ProcessID>
    <ProcessID> is shown when Started Tcp Server.

# [Uninstall(for Windows)]

    Open CMD as Admin
    # cd EventServer/SampleService/x64/Debug

    # Uninstall.exe