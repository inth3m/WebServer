#include"Timer.h"
#include<sys/time.h>
#include<unistd.h>
#include<queue>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout)
:   deleted_(false),
    sp_HttpData(requestData){
        struct timeval now;
        gettimeofday(&now, NULL);
        expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
    }

TimerNode::~TimerNode(){
    if(sp_HttpData)
        sp_HttpData->handleClose();
}

TimerNode::TimerNode(TimerNode &tn){
    sp_HttpData = tn.sp_HttpData;
}

void TimerNode::update(int timeout){
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid(){
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    if (temp < expiredTime_)
        return true;
    else
    {
        this->setDeleted();
        return false;
    }
}

void TimerNode::clearReq(){
    sp_HttpData.reset();
    this->setDeleted();
}

TimerManager::TimerManager(){

}

TimerManager::~TimerManager(){

}

void TimerManager::addTimer(std::shared_ptr<HttpData> sp_HttpData, int timeout){
    sp_TimerNode new_Node(new TimerNode(sp_HttpData,timeout));
    timerNodeQueue.push(new_Node);
    sp_HttpData->linkTimer(new_Node);
}

//
// (1) 优先队列不支持随机访问
// (2) 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
// 所以对于被置为deleted的时间节点，会延迟到它 <超时> 或 <它前面的节点都被删除>时，它才会被删除。
// 一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
// 这样做有两个好处：
// (1) 第一个好处是不需要遍历优先队列，省时。
// (2) 第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
// 就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次 delete和一次 new的时间。

void TimerManager::handleExpiredEvent(){
    while(!timerNodeQueue.empty()){
        sp_TimerNode ptime_now = timerNodeQueue.top();
        if(ptime_now->isDeleted()){
            timerNodeQueue.pop();
        }
        else if(ptime_now->isValid() == false){
            timerNodeQueue.pop();
        }
        else{
            break;
        }
    }
}