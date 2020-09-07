#include "file.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace logfind
{
    const int bufsize = 1024*1024;

    Buffer *CreateBuffer()
    {
        Buffer *pb = new Buffer();
        pb->bufsize = bufsize;
        pb->buffer = new char[bufsize];
        pb->offset = 0;
        pb->curpos = 0;
        pb->datalen = 0;
        return pb;
    }

    bool containsOffset(Buffer *pBuf, uint64_t offset)
    {
        if (offset >= pBuf->offset && offset < pBuf->offset + pBuf->datalen)
            return true;
        return false;
    }

    uint32_t getBufferOffsetFromFileOffset(Buffer *pBuf, uint64_t offset)
    {
        return offset - pBuf->offset;
    }
    
    ReadFile::ReadFile()
    :fd_(-1)
    ,buffer_(nullptr)
    ,offset_(0)
    ,lineno_(0)
    {
        buffer_ = CreateBuffer();
        prev_ = CreateBuffer();
        lines_[1] = 0;
    }

    ReadFile::~ReadFile()
    {
        delete[] buffer_;
        close(fd_);
    }

    bool ReadFile::open(const char *fname)
    {
        if (fname == nullptr)
        {
            fd_ = 0;
        }
        else
        {
            fd_ = ::open(fname, O_RDONLY);
            if (fd_ < 0)
                return false;
        }
        read_();
        return true;
    }

    Buffer *ReadFile::getBufferFromOffset(uint64_t offset)
    {
        if (containsOffset(buffer_, offset))
            return buffer_;
        if (containsOffset(prev_, offset))
            return prev_;
        return nullptr;
    }

    void ReadFile::readLine(uint64_t lineno, char *linebuf, uint32_t size)
    {
        uint64_t off = lines_[lineno];
        Buffer *pBuf = getBufferFromOffset(off);
        if (pBuf == nullptr)
        {
            strcpy(linebuf, "<not found>");
            return;
        }
        uint32_t bufpos = getBufferOffsetFromFileOffset(pBuf, off);
        const char *p = pBuf->buffer + bufpos;
        uint32_t count = bufpos;
        char *dest = linebuf;
        while (*p != '\n' && count < pBuf->datalen)
        {
            *dest++ = *p++;
            ++count;
        }
        *dest = '\0';
    }

    uint32_t ReadFile::bytesToRead()
    {
        uint32_t readsize = buffer_->bufsize - buffer_->datalen;
        if (readsize == 0)
        {
            std::swap(buffer_, prev_);
            buffer_->datalen = 0;
            buffer_->curpos = 0;
            buffer_->offset = offset_;
            return buffer_->bufsize;
        }
        return readsize;
    }

    void ReadFile::read_()
    {
        uint32_t readsize = bytesToRead();
        int r = read(fd_, buffer_->buffer+buffer_->datalen, readsize);
        if (r < 0)
        {
            buffer_->datalen = 0;
        }
        if (r >= 0)
            buffer_->datalen += r;
    }

    char ReadFile::get()
    {
        if (buffer_->datalen == 0)
            return 0;   // eof
        if (buffer_->curpos == buffer_->datalen)
        {
            read_();
        }
        if (buffer_->datalen == 0)
            return 0;   // eof
        ++offset_;
        char c = buffer_->buffer[buffer_->curpos++];
        if (c == '\n')
        {
            lines_[++lineno_] = offset_;
        }
        return c;
    }
}
