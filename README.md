# EventServer
Tcp or Udp Server/Clinet Project for my Studying C++

# [起動方法]
(1) サーバー
    #cd \Project Root\EventServer\Debug
    #TcpServer2.exe 5000
      (5000はポート番号。システムで使用していない任意の番号で可)

(2) クライアント
    #cd \Project Root\EventServer\Debug
    #ipconfig(IPv4アドレス調べる)
    #TcpClient.exe IPv4アドレス 5000

# [停止方法]
(1) サーバー
    #cd \Project Root\EventServer\Debug
    #Stop.exe stop
    もしくは、Ctrl + C

(2) クライアント
    「T子文字のアルファベットを入力してください」
　　の入力待ちに対して「.」(ピリオド)を入力すると終了する。

# [備考]
・TcpServerはクライアントと接続中に、他のクライアントからの
   接続要求に応答できます。