# EventServer
Tcp or Udp Server/Clinet Project for my Studying C++

# [起動方法]
(1) サーバー
    TcpServer 5000
      (5000はポート番号。システムで使用していない任意の番号で可)

(2) クライアント
    TcpClient 192.168.33.10 5000

# [停止方法]
(1) サーバー
    $ ps aux | grep -e TcpServer
    $ ./Stop TcpServerのスレッドid

(2) クライアント
    「T子文字のアルファベットを入力してください」
　　の入力待ちに対して「.」(ピリオド)を入力すると終了する。

# [備考]
・TcpServerはクライアントと接続中に、他のクライアントからの
   接続要求に応答できます。