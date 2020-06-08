#include "Buffer.h"
#include <sys/uio.h>
#include <errno.h>
#include <algorithm>
void Buffer::append(const char* data, size_t len)
{
    if (writableBytes() < len)      enlargeSpace(len);
    std::copy(data, data + len, begin() + writerIndex);
    writerIndex += len;
}

bool Buffer::readFd()
{
    char extrabuf[65536];
    struct iovec vec[2];
    int bytes_read = 0;
    while (true)
    {
        const size_t writable = writableBytes();
        vec[0].iov_base = begin() + writerIndex;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof extrabuf;
        const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
        bytes_read = ::readv(socketfd, vec, iovcnt);
        
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
                
            return false;
        }
        else if (bytes_read == 0)    return false;
        else if(static_cast<size_t>(bytes_read) <= writable)
        {
            writerIndex += bytes_read;
        }
        else {
            writerIndex = charVec.size();
            append(extrabuf, bytes_read - writable);
        }
    }
	charVec[writerIndex - 1] = '\r';
	charVec[writerIndex] = '\n';
	writerIndex += 1;
    return true;
}

void Buffer::enlargeSpace(size_t len)
{
    charVec.resize(writerIndex + len);
    assert(len <= writableBytes());
}

std::string Buffer::getMessageAsString()
{
    std::string message(begin(), readableBytes());
    clear();
    return message;
}

void Buffer::clear()
{
    writerIndex = 0;
    readerIndex = 0;
}
