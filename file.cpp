#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include "linebuf.h"
#include "buffer.h"
#include "lru_cache.h"
#include "file.h"
#include "application.h"

namespace logfind
{
    
    ReadFile::ReadFile()
    :pInput_(nullptr)
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
        if (pInput_)
            pInput_->close();
        delete pInput_;
    }

    void ReadFile::reset()
    {
        if (pInput_)
        {
            pInput_->close();
        }
        delete pInput_;
        cache_.clear();
        eof_ = false;
        lineno_ = 0;
        offset_ = 0;
        buffer_ = nullptr;
        lines_.clear();
        filename_.clear();
    }

    bool ReadFile::open(const char * f)
    {
        reset();

        filename_ = f;
        if (filename_.size() > 3 && strcmp(&filename_[filename_.size()-3], ".gz") == 0)
            pInput_ = new ZInput();
        else
            pInput_ = new TInput();
        if (pInput_->open(f) == false)
            return false;
        if (read_() < 0)
            return false;
        return true;
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
        int r = pInput_->read(buffer_->writePos(), buffer_->availableWriteBytes());
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
    TInput::~TInput()
    {
        ::close(fd_);
    }

    void TInput::close()
    {
        ::close(fd_);
    }

    bool TInput::open(const char *fname)
    {
        if (strcmp(fname, "-") == 0)
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

    int TInput::read(char *dest, uint64_t len)
    {
        return ::read(fd_, dest, len);
    }

    /**************************************************************/

    ZInput::~ZInput()
    {
        ::close(fd_);
    }

    void ZInput::close()
    {
        ::close(fd_);
    }

    bool ZInput::open(const char *fname)
    {
        memset(in_, 0, sizeof(in_));
        strm_.zalloc = Z_NULL;
        strm_.zfree = Z_NULL;
        strm_.opaque = Z_NULL;
        strm_.avail_in = 0;
        strm_.next_in = Z_NULL;

        int ret = inflateInit2(&strm_, 32+MAX_WBITS);
        if (ret != Z_OK)
        {
            return false;
        }

        if (strcmp(fname, "-") == 0)
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

    int ZInput::read(char *dest, uint64_t len)
    {
        int ret;
        if (strm_.avail_in == 0)
        {
            int r = ::read(fd_, in_, sizeof(in_));
            if (r <= 0)
            {
                inflateEnd(&strm_);
                ::close(fd_);
                return r;
            }

            strm_.avail_in = r;
            strm_.next_in = (Bytef *)in_;
        }
        strm_.avail_out = len;
        strm_.next_out = (Bytef *)dest;
        ret = inflate(&strm_, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
            case Z_NEED_DICT:
                std::cerr << "Need Dict error - " << std::endl;
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
                inflateEnd(&strm_);
                ::close(fd_);
                std::cerr << "Data Error" << std::endl;
                return ret;
            case Z_MEM_ERROR:
                inflateEnd(&strm_);
                ::close(fd_);
                std::cerr << "Out of memory" << std::endl;
                return ret;
        }
        return len - strm_.avail_out;
    }
}
