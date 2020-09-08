#include "buffer.h"

namespace logfind
{
    Buffer::Buffer()
    :bufsize(BUFSIZE)
    ,buffer(new char[BUFSIZE])
    ,offset(0)
    ,curpos(0)
    ,datalen(0)
    ,eob_(false)
    {
    }

    Buffer::~Buffer()
    {
        delete [] buffer;
    }

    bool Buffer::containsFileOffset(uint64_t fileoffset)
    {
        if (fileoffset >= offset && fileoffset < offset + datalen)
            return true;
        return false;
    }

    uint32_t Buffer::getBufferOffsetFromFileOffset(uint64_t fileoff)
    {
        return fileoff - offset;
    }

    void Buffer::readline(uint64_t fileoffset, char *linebuf, uint32_t bufsize)
    {
        uint32_t bufpos = getBufferOffsetFromFileOffset(fileoffset);
        const char *p = buffer + bufpos;
        uint32_t count = bufpos;
        char *dest = linebuf;
        while (*p != '\n' && count < datalen)
        {
            *dest++ = *p++;
            ++count;
        }
        *dest = '\0';
    }

    bool Buffer::eob()
    {
        return eob_;
    }

    char Buffer::getchar()
    {
        if (curpos == datalen)
        {
            eob_ = true;
            return 0;
        }
        return buffer[curpos++];
    }

    uint64_t Buffer::fileoffset()
    {
        return offset;
    }

    uint32_t Buffer::availableBytes()           
    {
        return bufsize-datalen;
    }

    char *Buffer::bufpos()                      
    {
        return buffer+curpos;
    }

    void Buffer::datasize(uint32_t n)           
    {
        datalen += n;
    }

    bool Buffer::isFull()                       
    {
        return datalen==bufsize;
    }

    void Buffer::reset(uint64_t fileoffset)
    {
        datalen = 0;
        curpos = 0;
        offset = fileoffset;
        eob_ = false;
    }
}
