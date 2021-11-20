# EventServer
Tcp Server Windows Service. Thread Pool using Boost.Asio.

# [Launch System]
(1) Install the "SampleServe" as a Windows Service(First time only)

(2) Start the "SampleServ" Service

(3) Client
    #cd \Project Root\x64\Debug
    #ipconfig(Get Local IPv4 address)
    #TcpClient.exe IPv4 address 5000

# [Stop]
(1) Stop Server
    Command Prompt as Admin
    #cd \Project Root\x64\Debug
    #Stop.exe stop
    Or Ctrl + C

(2) Stop Client
    「T子文字のアルファベットを入力してください」
　　-> Type just ".", then press enter key
  
(3) Stop the "SampleServ" Service
