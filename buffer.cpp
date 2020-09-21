#include <cstdint>
#include "application.h"
#include "linebuf.h"
#include "buffer.h"

namespace logfind
{
    Buffer::Buffer()
    :bufsize_(BUFSIZE)
    ,buffer_(new char[BUFSIZE])
    ,offset_(0)
    ,readpos_(0)
    ,datalen_(0)
    {
    }

    Buffer::~Buffer()
    {
        delete [] buffer_;
    }

    bool Buffer::containsFileOffset(uint64_t fileoffset)
    {
        if (fileoffset >= offset_ && fileoffset < offset_ + datalen_)
            return true;
        return false;
    }

    uint32_t Buffer::getBufferOffsetFromFileOffset(uint64_t fileoff)
    {
        if (containsFileOffset(fileoff) == false)
            return (uint32_t)-1;
        return fileoff - offset_;
    }

    bool Buffer::readline(uint64_t fileoffset, linebuf& lb)
    {
        theApp->alloc(lb);
        uint32_t bufpos = getBufferOffsetFromFileOffset(fileoffset);
        const char *p = buffer_ + bufpos;
        uint32_t count = bufpos;
        char *dest = lb.buf;
        uint32_t numwriten(0);
        while (*p != '\n' && count < datalen_ && numwriten < lb.bufsize)
        {
            *dest++ = *p++;
            ++count;
            ++numwriten;
        }
        *dest = '\0';
        lb.len = numwriten;
        return true;
    }

    bool Buffer::eob()
    {
        return availableReadBytes() == 0;
    }

    char Buffer::getchar()
    {
        if (readpos_ == datalen_)
        {
            return 0;
        }
        return buffer_[readpos_++];
    }

    uint64_t Buffer::fileoffset()
    {
        return offset_;
    }

    uint32_t Buffer::availableWriteBytes()           
    {
        return bufsize_-datalen_;
    }

    uint32_t Buffer::availableReadBytes()           
    {
        return datalen_ - readpos_;
    }

    bool Buffer::setReadPos(const char *p)
    {
        if (p < buffer_ || p >= &buffer_[datalen_])
            return false;
        readpos_ = p - buffer_;
        return true;
    }

    void Buffer::incrementReadPosition(uint32_t n)
    {
        readpos_ += n;
    }

    char *Buffer::readPos()
    {
        return buffer_+readpos_;
    }

    char *Buffer::writePos()
    {
        return buffer_+datalen_;
    }

    void Buffer::incrementAvailableReadBytes(uint32_t n)           
    {
        datalen_ += n;
    }

    bool Buffer::isFull()                       
    {
        return availableWriteBytes() == 0;
    }

    void Buffer::reset(uint64_t fileoffset)
    {
        datalen_ = 0;
        readpos_ = 0;
        offset_ = fileoffset;
    }
}
