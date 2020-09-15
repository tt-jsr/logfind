#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include "linebuf.h"
#include "buffer.h"
#include "lru_cache.h"
#include "file.h"
#include "application.h"

namespace logfind
{
    
    ReadFile::ReadFile()
    :pInput(nullptr)
    ,buffer_(nullptr)
    ,offset_(0)
    ,lineno_(0)
    ,cache_(10)
    ,eof_(false)
    {
        lines_[0] = 0;
    }

    ReadFile::~ReadFile()
    {
        if (pInput)
            pInput->close();
        delete pInput;
    }

    void ReadFile::attach(Input *p)
    {
        pInput = p;
        read_();
    }

    std::string ReadFile::filename()
    {
        return filename_;
    }

    Buffer *ReadFile::getBufferFromFileOffset(uint64_t offset)
    {
        uint64_t page = (offset/BUFSIZE) * BUFSIZE;
        return cache_.get(page);
    }

    bool ReadFile::readLine(uint64_t lineno, linebuf& lb)
    {
        uint64_t off = lines_[lineno];
        Buffer *pBuf = getBufferFromFileOffset(off);
        if (pBuf == nullptr)
        {
            theApp->alloc(lb);
            strcpy(lb.buf, "<not found>");
            lb.len = strlen(lb.buf);
            return true;
        }
        // Buffer may not contain the entire line
        return pBuf->readline(off, lb);
    }

    int ReadFile::read_()
    {
        if (buffer_ == nullptr || buffer_->isFull())
        {
            buffer_ = cache_.get_lru();
            buffer_->reset(offset_);
            cache_.add(buffer_);
        }
        int r = pInput->read(buffer_->writePos(), buffer_->availableWriteBytes());
        if (r <= 0)
            eof_ = true;
        if (r > 0)
            buffer_->incrementAvailableReadBytes(r);
        return r;
    }

    bool ReadFile::eof()
    {
        return eof_;
    }

    char ReadFile::get()
    {
        char c = buffer_->getchar();
        if (c == 0)
        {
            if (read_() <= 0)
            {
                return 0;
            }
            c = buffer_->getchar();
        }
        ++offset_;
        if (c == '\n')
        {
            lines_[++lineno_] = offset_;
        }
        return c;
    }

    /**************************************************************/
    TFile::~TFile()
    {
        ::close(fd_);
    }

    void TFile::close()
    {
        ::close(fd_);
    }

    bool TFile::open(const char *fname)
    {
        if (fname == nullptr || fname[0] == '\0')
        {
            fd_ = 0;
        }
        else
        {
            fd_ = ::open(fname, O_RDONLY);
            if (fd_ < 0)
                return false;
            filename_ = fname;
        }
        return true;
    }

    int TFile::read(char *dest, uint64_t len)
    {
        return ::read(fd_, dest, len);
    }

    std::string TFile::filename()
    {
        return filename_;
    }
}
