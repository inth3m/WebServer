//封装FileUtil

#ifndef BASE_LOGFILE_H
#define BASE_LOGFILE_H

#include"FileUtil.h"
#include"MutexLock.h"
#include"noncopyable.h"
#include<memory>
#include<string>

class LogFile:noncopyable{
public:
    //每次append flushEveryN 次,就会刷新一下缓冲区
    LogFile(const std::string &basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    const std::string basename_;
    const int flushEveryN_;
    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;

};

#endif