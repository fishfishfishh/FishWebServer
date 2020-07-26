#pragma once
#include <vector>
#include <assert.h>
#include <iostream>
class Buffer
{
public:
    static const size_t kInitialSize = 1024;
public:
    explicit Buffer(size_t initialSize = kInitialSize) : charVec(initialSize),
        readerIndex(0),
        writerIndex(0)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
    }
    void setSocketfd(int fd) { socketfd = fd; }
    size_t readableBytes() const
    {
        return writerIndex - readerIndex;
    }
    size_t writableBytes() const
    {
        return charVec.size() - writerIndex;
    }
    char* begin()
    {
        return &*charVec.begin();
    }

    const char* begin() const
    {
        return &*charVec.begin();
    }
    char* beginRead() 
    {
        return begin() + readerIndex;
    }
	char* end()
	{
		return &*charVec.end();
	}
	const char* end() const
	{
		return &*charVec.end();
	}
    const char* beginRead() const 
    {
        return begin() + readerIndex;
    }
    void retrieve(size_t len) {
        assert(len <= readableBytes());
        if (len < readableBytes())   readerIndex += len;
        else    clear();
    }
    void append(const char* data, size_t len);
    bool readFd();
    void enlargeSpace(size_t len);
    std::string getMessageAsString();
    void clear();
private:
    std::vector<char> charVec;
    size_t readerIndex;
    size_t writerIndex;
    int socketfd;
};

