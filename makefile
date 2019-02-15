objects=main.o Server.o Http_conn.o Mimetype.o EventLoop.o Epoll.o Channel.o TimerManager.o Logging.o AsyncLogging.o LogStream.o LogFile.o FileUtil.o ThreadpoolEventLoop.o ThreadEventLoop.o Thread.o Packet.o Conf.o MemoryPool.o LFUCache.o 

origins=main.cc HTTPServer/Server.cc HTTPServer/Http_conn.cc HTTPServer/Mimetype.cc HTTPServer/Packet.cc Reactor/EventLoop.cc Reactor/Epoll.cc Reactor/Channel.cc Reactor/TimerManager.cc Log/Logging.cc Log/AsyncLogging.cc Log/LogStream.cc Log/LogFile.cc Log/FileUtil.cc ThreadPool/ThreadpoolEventLoop.cc ThreadPool/ThreadEventLoop.cc ThreadPool/Thread.cc conf/Conf.cc MemoryPool/MemoryPool.cc cache/LFUCache.cc

g11=g++ -std=c++11
 
path=./test

WebServer:$(objects)
	$(g11) -o WebServer $(objects) -lpthread

$(objects):$(origins)
	$(g11) -g -c $(origins)

.PHONY:clean
clean:
	rm WebServer $(objects)

.PHONY:install
install:
	mkdir $(path)
	mkdir $(path)/log
	mkdir $(path)/conf
	mv WebServer $(path)
	cp conf/WebServer.conf $(path)/conf
	cp -r page $(path)/page
	rm *.o
