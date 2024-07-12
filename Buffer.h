#pragma once
#include <vector>
#include <string>
#include <assert.h>
#include <sys/types.h>

// buffer的数据结构本质是vector<char>，目的是为了能随时动态调整空间大小
// buffer_的布局如下
// +-------------------+-------------------+-------------------+
// | prependable bytes |  readable bytes   |  writable bytes   |
// |                   |     (CONTENT)     |                   |
// +-------------------+-------------------+-------------------+
// |                   |                   |                   |
// |        <=     readIndex_   <=   writeIndex_    <=    buffer_.size()
// |--------+--------------------------------------------------+
// 0        8                                                1024
// |kCheapPr|                  kInitialSize                    |
//
// kCheapPrepend：预留8字节头部防止越界
// kInitialSize：初始缓冲区负荷长度
// prependable bytes：已发送完数据的空间，可以挪移数据prepend到该空间
// readable bytes: 待发送数据的空间
// writable bytes：待填充数据的空间
// 该buffer_只能序列化存储数据，不允许碎片化存储，碎片化存储的代码可读性降低，性能优化效果不是特别显著
// 每次发送完全部数据后(readIndex_==writeIndex_)，就将该两指针全部移至kCheapPrepend位置

class Buffer
{
public:
    static const int kCheapPrepend = 8;
    static const int kInitialSize = 1024;
    Buffer()
        : buffer_(kCheapPrepend + kInitialSize),
          readIndex_(kCheapPrepend),
          writeIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }
    /* 读取缓冲区待发送数据的起始位置 */
    const char *peek() const { return buffer_.data() + readIndex_; }

    /* 每次填充完数据就调用该函数调整writeIndex_位置 */
    void hasWritten(size_t len)
    {
        writeIndex_ += len;
    }

    /* 确保缓冲区有足够空间填充数据，否则就扩增vector空间至刚好适配数据大小 */
    void ensureWritable(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    /* 填充数据时调用该函数 */
    void append(const char *data, size_t len)
    {
        ensureWritable(len);
        std::copy(data, data + len, buffer_.data() + writeIndex_);
        hasWritten(len);
    }

    /* 任意数据类型的重载版本 */
    void append(const void *data, size_t len)
    {
        append(static_cast<const char *>(data), len);
    }

    /* string类型的重载版本 */
    void append(const std::string &str)
    {
        append(str.data(), str.length());
    }

    /* 发送完数据后调用该函数调整readIndex_位置 */
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        readIndex_ += len;
    }

    void retrieveAll()
    {
        readIndex_ = writeIndex_ = kCheapPrepend;
    }

    /* 一次性发送所有数据 */
    std::string retrieveAsString()
    {
        std::string str(peek(), readableBytes());
        retrieveAll();
        return str;
    }

    /* 将数据prepend到缓冲区前面空闲的空间 */
    void prepend(const void *data, size_t len)
    {
        assert(len <= prependableBytes());
        readIndex_ -= len;
        const char *char_data = static_cast<const char *>(data);
        std::copy(char_data, char_data + len, buffer_.data() + readIndex_);
    }

    /* 调整缓冲区大小 */
    void shrink(size_t reserve)
    {
        std::vector<char> new_buffer(kCheapPrepend + readableBytes() + reserve);
        std::copy(peek(), peek() + readableBytes(), new_buffer.data() + kCheapPrepend);
        buffer_.swap(new_buffer);
    }

    /* 从套接字一次性读取数据到缓冲区中，缓存空间（栈上开辟空间实现分散读）足够大以便不会阻塞在IO上 */
    ssize_t readFd(int fd, int *savedErrno);

private:
    /* 不够缓冲数据时，就调整vector空间以恰好适配数据大小 */
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};