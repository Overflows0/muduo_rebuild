#include "Buffer.h"
#include "sys/uio.h"

ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    char extrabuf[65536]; // 栈上开辟空间实现分散读
    struct iovec vec[2];
    const size_t writable = writableBtyes();
    vec[0].iov_base = buffer_.data() + writeIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const ssize_t n = ::readv(fd, vec, 2); // 一次性读取所有数据，而不会因为开辟新的vector空间阻塞在IO上
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        writeIndex_ += n;
    }
    else
    {
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable); // 此时已经完成IO事务，就可以做开辟空间的工作
    }
    return n;
}

void Buffer::makeSpace(size_t len)
{
    if (len + kCheapPrepend > writableBtyes() + prependableBtyes())
    {
        buffer_.resize(writableBtyes() + len);
    }
    /* 如果原缓冲区的空闲空间足够储存，就挪移待发送数据 */
    else
    {
        assert(kCheapPrepend < readIndex_);
        size_t readable = readableBtyes();
        std::copy(buffer_.data() + readIndex_,
                  buffer_.data() + writeIndex_,
                  buffer_.data() + kCheapPrepend);
        readIndex_ = kCheapPrepend;
        writeIndex_ = readIndex_ + readable;
        assert(readable == readableBtyes());
    }
}