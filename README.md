# Smart NIC Heartbeat

Smart NICを使用し、ホストが高負荷でheartbeatを送れない場合でもheartbeatが送られるようにする。また、heartbeatメッセージにホストの稼働状況を乗せることで、適切にタスクを分散できるようにする。

## Manager

Heartbeatのリクエストを送り、動いているかを監視する。

## Subsystem

Smart NIC上で動くプログラム。Controllerから情報をもらい、Heartbeatを送信する。

## Controller

監視される先で動かすプログラム。カーネル内部を覗き、Subsystemに情報をあげる。

## 実行
サーバは，
./controller2
を実行する．


