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

    bool Buffer::containsFileOffset(F_OFFSET fileoffset)
    {
        if (fileoffset >= offset_ && fileoffset < offset_ + datalen_)
            return true;
        return false;
    }

    B_OFFSET Buffer::getBufferOffsetFromFileOffset(F_OFFSET fileoff)
    {
        if (containsFileOffset(fileoff) == false)
            return (B_OFFSET)-1;
        return fileoff - offset_;
    }

    bool Buffer::readline(F_OFFSET fileoffset, linebuf& lb)
    {
        theApp->alloc(lb);
        B_OFFSET bufpos = getBufferOffsetFromFileOffset(fileoffset);
        const char *p = buffer_ + bufpos;
        size_t count = bufpos;
        char *dest = lb.buf;
        size_t numwriten(0);
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

    F_OFFSET Buffer::fileoffset()
    {
        return offset_;
    }

    size_t Buffer::availableWriteBytes()           
    {
        return bufsize_-datalen_;
    }

    size_t Buffer::availableReadBytes()           
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

    void Buffer::incrementReadPosition(size_t n)
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

    void Buffer::incrementAvailableReadBytes(size_t n)           
    {
        datalen_ += n;
    }

    bool Buffer::isFull()                       
    {
        return availableWriteBytes() == 0;
    }

    void Buffer::reset(F_OFFSET fileoffset)
    {
        datalen_ = 0;
        readpos_ = 0;
        offset_ = fileoffset;
    }
}
