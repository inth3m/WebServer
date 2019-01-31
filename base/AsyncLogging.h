#ifndef BASE_ASYNCLOGGING_H
#define BASE_ASYNCLOGGING_H
#include"MutexLock.h"
#include"noncopyable.h"
#include"CountDownLatch.h"
#include"Thread.h"
#include"LogStream.h"
#include<functional>
#include<string>
#include<vector>

class AsyncLogging:noncopyable{
public:
    AsyncLogging(const std::string baseName, int flushInterval = 2);
    ~AsyncLogging(){
        if(running_){
            stop();
        }
    }
    void append(const char* logline, int len);

    void start(){
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop(){
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    private:
    void threadFunc();
    typedef FixeBuffer<LargeBuffer> Buffer;
    typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
    typedef std::shared_ptr<Buffer> BufferPtr;
    const int flushInterval_;
    bool running_;
    std::string basename_;
    Thread thread_;
    MutexLock mutex_;               
    Condition cond_;
    BufferPtr currentBuffer_;       //当前缓冲区
    BufferPtr nextBuffer_;          //预备缓冲区
    BufferVector buffers_;          //待写入文件文件的已填满的缓冲区,vector存的是智能指针
    CountDownLatch latch_;
};
#endif