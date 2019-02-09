#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>


AsyncLogging::AsyncLogging(std::string logFileName, int flushInterval)
:   flushInterval_(flushInterval),
    running_(false),
    basename_(logFileName),
    thread_(std::bind(&AsyncLogging::threadFunc,this),"loging"),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(),
    latch_(1){
        assert(logFileName.size()>1);
        currentBuffer_->bzero();
        nextBuffer_->bzero();
        buffers_.reserve(16);
    }

//前端生成一条日志消息的时候调用一次
void AsyncLogging::append(const char* logline, int len){
    MutexLockGuard lock(mutex_);
    //当前缓冲区还没满
    if(currentBuffer_->avail() >len)
        currentBuffer_->append(logline, len);
    //缓冲区已满
    else{
        //把当前缓冲区移入bufers_
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        //把另一个缓冲区move 为当前缓冲区
        if(nextBuffer_)
            //交换指针操作
            currentBuffer_ = std::move(nextBuffer_);
        else
            currentBuffer_.reset(new Buffer);
        currentBuffer_->append(logline, len);
        //唤醒后端写数据
        cond_.notify();
    }
}

void AsyncLogging::threadFunc(){
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);
    //准备好两块空Buffer,以备在临界区交换
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);

    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while(running_){
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if(buffers_.empty()){
                //等待条件有两个:超时和前端写满了一个Buffer
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            //把newBuffer1 置为当前缓冲区
            currentBuffer_ = std::move(newBuffer1);
            //将buffers_与 buffersToWrite 进行交换
            buffersToWrite.swap(buffers_);
            //用newBuffer2 替换nextBuffer, 保证nextBuffer 可用
            if (!nextBuffer_){
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        //buffersToWrite 写入
        assert(!buffersToWrite.empty());
        if (buffersToWrite.size() > 25){

            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }
        for (size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            // FIXME: use unbuffered stdio FILE ? or use ::writev ?
            output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }

        if (buffersToWrite.size() > 2)
        {
            // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        }

        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}