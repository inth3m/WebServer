#include "Epoll.h"
#include "Util.h"
#include "base/Logging.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <queue>
#include <deque>
#include <assert.h>
#include <arpa/inet.h>
#include <iostream>

using namespace std;

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

typedef shared_ptr<Channel> sp_Channel;

Epoll::Epoll():
    epollFd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(EVENTSNUM){
    assert(epollFd_ > 0);
}
Epoll::~Epoll()
{ }

//注册新描述符
void Epoll::epoll_add(sp_Channel request, int timeout){
    int fd = request->getFd();
    if(timeout>0){
        add_timer(request, timeout);
        fd2http_[fd] = request->getHolder();
    }
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    request->EqualAndUpdateLastEvents();

    fd2chan_[fd] = request;
    if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event)<0){
        perror("epoll_add error");
        fd2chan_[fd].reset();
    }
}

//修改描述符状态
void Epoll::epoll_mod(sp_Channel request, int timeout){
    if (timeout > 0)
        add_timer(request, timeout);
    int fd = request->getFd();
    if (!request->EqualAndUpdateLastEvents()){
        struct epoll_event event;
        event.data.fd = fd;
        event.events = request->getEvents();
        if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0){
            perror("epoll_mod error");
            fd2chan_[fd].reset();
        }
    }
}

// 从epoll从中删除描述符

void Epoll::epoll_del(sp_Channel request){
    int fd = request->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0){
        perror("epoll_del error");
    }
    fd2chan_[fd].reset();
    fd2http_[fd].reset();
}