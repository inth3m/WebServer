#ifndef BASE_COUNTDOWNLATCH_H
#define BASE_COUNTDOWNLATCH_H
#include"Condition.h"
#include"MutexLock.h"
#include"noncopyable.h"

//CountDownLatch 的作用是确保thread中传进去的func真的启动之后
//外层的start才返回

class CountDownLatch : noncopyable{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

private:
    //易变变量
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;

};

#endif