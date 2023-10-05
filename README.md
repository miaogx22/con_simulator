# 并发测试模拟器(con_simulator)

## 1. 概述

本项目主要用于执行TCP长连接的并发测试. 基于TCP的开发工程师在没有测试仪的情况下可以作为临时的性能测试工具使用，分为客户端和服务端。采用clion作为开发工具。

客户端程序为:client_simulator, 命令行方式执行,运行于Linux之上,可以绑定本地的多个IP连接多个服务器IP及端口.最多为100个.

服务端程序为:server_simulator, 命令行方式执行,运行于Linux之上,可以绑定本地的多个IP监听多个端口. 最多为100个.

## 2. 使用

### 2.1 服务端

#### 2.1.1 ip:port文件

首先，你需要准备好ip_list文件，名称无所谓，后缀名无所谓，但是一定要是文本文件, server_simulator将通过-f来读入该文件, 好能绑定指定的IP:Port对来监听，其格式如下：

```
127.0.0.1:10001
127.0.0.1:10002
127.0.0.1:10003
```

IP地址与端口使用冒号:分隔,此文件中不允许出现其他数据内容.

#### 2.1.2 运行server_simulator

##### 参数说明

```
server_simulator
-h:            查看帮助选秀
-v:            查看版本
-f:            载入ip:port文件
-c:            定义每个ip:port单元的最大支持并法数
-e:            设置数据回显，如果指定次选项, 则服务器将执行回写客户端发过来的
-i:            指定当收到客户发送的数据时的回复数据的读取文件, 注意文件内容不能超过1k。
```

##### 运行实例

```
./server_simulator -f ip_list_server -c 100000               //简单的运行实例
./server_simulator -f ip_list_server -c 100000 -e            //收到数据直接回写
./server_simulator -f ip_list_server -c 100000 -i send_file  //由文件读取数据发生给客户端
```

##### 其他

暂无



### 2.2 客户端

#### 2.2.1 ip:port文件

首先，你同样需要准备好ip_list文件，名称无所谓，后缀名无所谓，但是一定要是文本文件, client_simulator将通过-f来读入该文件, 好能绑定指定的本地IP地址来连接指定的服务器地址，其格式如下：

```
0.0.0.0:192.168.1.100:10001
192.168.1.2:192.168.1.100:10002
192.168.1.2::192.168.1.100:10003
```

冒号分隔的第一列是本地的IP地址, 第二列是服务器的IP地址, 第三列是服务器端口, 此文件中不允许出现其他数据内容.

#### 2.1.2 运行client_simulator

##### 参数说明

```
client_simulator
-h:            查看帮助选项
-v:            查看版本
-f:            载入ip:port文件
-c:            定义每个ip:port单元的最大支持并法数
-m:            指定连接建立的时间间隔，单位为毫秒, 如果不指定的话, 默认为最大建立频率，建议是设置为10
-s:            指定测试数据发送的时间间隔, 单位为秒, 如果不指定的话, 不发送测试数据
-i:            指定由文件读取测试数据的内容，不能超过1K，如果不指定且-s打开的时候，将发送默认的"1234567890".
```

##### 运行实例

```
./client_simulator -f ip_list_client -c 50000                //最简单的方式
./client_simulator -f ip_list_client -c 50000 -m 10          //每个并发的建立间隔为10个毫秒
./client_simulator -f ip_list_client -c 50000 -s 100         //并发建立后每隔100S发送一次数据
./client_simulator -f ip_list_client -c 50000 -m 10 -s 100 -i input_file       //每个并发的建立间隔为10个毫秒, 并发建立后每隔100S发送一次数据, 并且数据由input_file里面来获取
```



## 3. 项目开发

本项目是使用clion作为开发环境而开发的。

release目录下面的client和server目录里面是在ubuntu下面编译的。

## 4. 原理概述

客户端与服务端均采用epoll实现的多路复用，单进程多线程运行。