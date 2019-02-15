# WebServer

## Introduction

本项目是C++11编写的Web服务器,用状态机解析了http请求,实现了异步日志，记录服务器运行状态。

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