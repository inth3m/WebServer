# WebServer

## Introduction

本项目是C++11编写的Web服务器,用状态机解析了http请求,实现了异步日志，记录服务器运行状态。

[测试](https://github.com/inth3m/WebServer/blob/master/%E6%B5%8B%E8%AF%95.md)

## Environment

- OS:Manjaro 18.0.2 Illyria
- Complier:gcc 版本 8.2.1 20181127 (GCC)

## Build

```bash
make
make install
```

## Technical points

- 使用Epoll边沿触发的IO多路复用技术，非阻塞IO，使用Reactor模式
- 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
- 使用小根堆+unordered_map实现定时器以关闭超时连接
- 用状态机解析了http请求
- 使用RAII机制封装锁，让线程更安全
- 实现自己的内存池，进一步优化性能
- 参考muduo用双缓冲技术实现Log日志.


## 并发模型

&nbsp;&nbsp;&nbsp;&nbsp;程序使用Reactor模型，并使用多线程提高并发度。为避免线程频繁创建和销毁带来的开销，使用线程池，在程序的开始创建固定数量的线程。使用epoll作为IO多路复用的实现方式。

### Reactor模式

事件循环loop->封装EPOLL_WAIT的poll->返回事件集handles->对每个handle执行事件handleEvent->handleEvent通过回调函数执行

![Screenshot_20190215_143320.png](https://i.loli.net/2019/02/15/5c665d74782dc.png)
![Screenshot_20190215_143720.png](https://i.loli.net/2019/02/15/5c665e210f01f.png)

MainReactor只有一个，负责响应client的连接请求，并建立连接，它使用一个NIO Selector。在建立连接后用Round Robin的方式分配给某个SubReactor,因为涉及到跨线程任务分配，需要加锁，这里的锁由某个特定线程中的loop创建，只会被该线程和主线程竞争。

SubReactor可以有一个或者多个，每个subReactor都会在一个独立线程中运行，并且维护一个独立的NIO Selector。

### 连接的维护

#### 建立连接

- 建立连接的过程

连接的建立比较简单，server端通过socket()，bind()，listen()，并使用epoll ET模式监听listenfd的读请求，当TCP连接完成3次握手后，会触发listenfd的读事件，应用程序调用accept()，会检查已完成的连接队列，如果队列里有连接，就返回这个连接，出错或连接为空时返回-1。此时，已经可以进行正常的读写操作了。 当然，因为是ET模式，accept()要一直循环到就绪连接为空。

#### 连接的关闭

相对于连接的建立，关闭连接则复杂的多，远不是一个close()那么简单，关闭连接要优雅。

##### 什么时候关闭连接？

通常server和client都可以主动发Fin来关闭连接

- 对于client(非Keep-Alive)，发送完请求后就可以shutdown()写端，然后收到server发来的应答，最后close掉连接。也可以不shutdown()写，等读完直接close。对于Keep-Alive的情况，就要看client的心情了，收到消息后可以断，也可以不断，server应该保证不主动断开。

- 对于server端，毫无疑问应该谨慎处理以上所有情况

(1) 出现各种关于连接的错误时，可以直接close()掉
(2) 短连接超时的请求，可以close()，也可以不关
(3) 长连接对方长时间没有请求(如果没有保活机制)，可以close()，也可以不关
(4) client发出Fin，server会收到0字节，通常不能判断client是close了还是shutdown，这时server应当把消息发完，然后才可以close()，如果对方调用的是close，会收到RST，server能感知到，就可以立即close了

### 定时器

Timer里有两个数据结构：unordered_map和priority_queue，优先队列存放着TimerNode节点，只要节点的时间到了，或者该节点对应的channel事件关闭了那么该节点就可以删除，同时unorder_map是fd映射TimerNode节点，此TimerNode节点为fd对应的真正的节点，如果priority_queue删除的节点对应的不是unorder_map映射的节点，则证明当前的priority_queue里的节点并非真正的节点，故把堆顶的节点删除后要把unordered_map节点放进堆里。


## 内存池

&nbsp;&nbsp;&nbsp;&nbsp;结合STL的内存实现. 内存池维护了一个free_list,每一块都是8的倍数,从8开始,根据需要找不同的块,当块内存不足时通过malloc申请一块大内存

*图片来自STL源码剖析*

![Screenshot_20190215_142508.png](https://i.loli.net/2019/02/15/5c665b4580e6c.png)

![Screenshot_20190215_140027.png](https://i.loli.net/2019/02/15/5c6655741895f.png)
