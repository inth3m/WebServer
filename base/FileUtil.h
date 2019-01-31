#ifndef BASE_FILEUTIL_H
#define BASE_FILEUTIL_H
#include"noncopyable.h"
#include<string>

class AppendFile:noncopyable{
public:
    explicit AppendFile(std::string filename);
    ~AppendFile();
    //向文件写数据
    void append(const char* logline, const size_t len);
    void flush();
private:
    size_t write(const char *logline, size_t len);
    FILE* fp_;
    char buffer_[64*1024];
};
#endif