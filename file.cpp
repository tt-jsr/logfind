#include <string>
#include <unordered_map>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <thread>
#include "linebuf.h"
#include "buffer.h"
#include "lru_cache.h"
#include "file.h"
#include "application.h"

namespace logfind
{
    static const int NPAGES = 40;
    static const int NREAD_AHEAD = 20;
    
    ReadFile::ReadFile()
    :pInput_(nullptr)
    ,buffer_(nullptr)
    ,cache_(NPAGES)
    ,bRun_(false)
    ,read_ahead_(0)
    ,next_read_ahead_buffer_key_(0)
    {
    }

    ReadFile::~ReadFile()
    {
        if (pInput_)
            pInput_->close();
        delete pInput_;
        bRun_ = false;
        cv_.notify_one();
        if (thrd_.joinable())
            thrd_.join();
    }

    void ReadFile::reset()
    {
        if (pInput_)
        {
            pInput_->close();
        }
        delete pInput_;
        pInput_ = nullptr;
        buffer_ = nullptr;
        filename_.clear();
        cache_.clear();
        next_read_ahead_buffer_key_ = 0;
        read_ahead_ = 0;
        buffer_ = nullptr;
    }

    bool ReadFile::open(const char * f)
    {
        bRun_ = false;
        cv_.notify_one();
        if (thrd_.joinable())
            thrd_.join();
        {
            std::lock_guard<std::mutex> lck(mtx_);
            reset();

            filename_ = f;
            if (filename_.size() > 3 && strcmp(&filename_[filename_.size()-3], ".gz") == 0)
                pInput_ = new ZInput();
            else
                pInput_ = new TInput();
            if (pInput_->open(f) == false)
                return false;

            thrd_ = std::thread (&ReadFile::read_ahead_thread_, this);
        }
        // Wait until we have some read ahead
        while (read_ahead_ == 0)
            std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }

    std::string ReadFile::filename()
    {
        return filename_;
    }

    bool ReadFile::readLine(F_OFFSET offset, linebuf& lb)
    {
        //std::cerr << "===JSR readline" << std::endl;
        uint64_t key = fileOffsetToBufferKey(offset);
        std::lock_guard<std::mutex> lck(mtx_);
        Buffer *pBuf = cache_.get(key);
        if (pBuf)
        {
            bool r = pBuf->readline(offset, lb);
            cache_.add(pBuf);   // put it back
            return r;
        }
        theApp->alloc(lb);
        strcpy(lb.buf, "logfind: buffer not in cache!");
        lb.len = strlen(lb.buf);
        return true;
    }

    void ReadFile::read_ahead_thread_()
    {
        bRun_ = true;
        while (bRun_)
        {
            Buffer *pBuf(nullptr);
            {
                std::unique_lock<std::mutex> lck(mtx_);
                cv_.wait(lck,[this] {
                        if (this->bRun_ == false)
                            return true;
                        if (this->read_ahead_ < NREAD_AHEAD)
                            return true;
                        return false;
                    });
                if (bRun_ == false)
                    return;
                pBuf = cache_.get_lru();
            }
            assert(pBuf);
            pBuf->reset(next_read_ahead_buffer_key_);
            //std::cerr << "===JSR buffer key: " << next_read_ahead_buffer_key_ << std::endl;
            next_read_ahead_buffer_key_ = fileOffsetToBufferKey(next_read_ahead_buffer_key_+BUFSIZE+100);

            //std::cerr << "===JSR next key: " << next_read_ahead_buffer_key_ << std::endl;

            while (!pBuf->isFull())
            {
                if (bRun_ == false)
                    return;
                int r = pInput_->read(pBuf->writePos(), pBuf->availableWriteBytes());
                if (r <= 0)
                {
                    break;
                }
                if (r > 0)
                {
                    pBuf->incrementAvailableReadBytes((size_t)r);
                }
            }
            {
                std::lock_guard<std::mutex> lck(mtx_);
                ++read_ahead_;
                //std::cerr << "===JSR adding to cache: " << pBuf->fileoffset() << std::endl;
                cache_.add(pBuf);
            }
        }
    }

    bool ReadFile::buffer_check()
    {
        if (buffer_ == nullptr || buffer_->availableReadBytes() == 0)
        {
            uint64_t key(0);
            if (buffer_ == nullptr)
                key = 0;
            else
            {
            //std::cerr << "===JSR cache key prior " << buffer_->fileoffset() << std::endl;
                key = fileOffsetToBufferKey(buffer_->fileoffset()+BUFSIZE+100); 
            }

            //std::cerr << "===JSR cache key want " << key << std::endl;
            std::lock_guard<std::mutex> lck(mtx_);
            Buffer *pBuf = cache_.get(key);
            if (pBuf == nullptr)
                return false;
            assert(pBuf->fileoffset() == key);
            buffer_ = pBuf;
            cache_.add(buffer_);  // put it back in the cache
            //std::cerr << "===JSR current_buffer: " << buffer_->fileoffset() << std::endl;
            --read_ahead_;
            cv_.notify_one();
        }
        return true;
    }

    int ReadFile::read(char *buf, int size)
    {
        if (!buffer_check())
            return 0;
            
        uint32_t b = buffer_->availableReadBytes();
        if (b < size)
            size = b;
        memcpy(buf, buffer_->readPos(), size);
        buffer_->incrementReadPosition(size);
        return size;
    }

    Buffer *ReadFile::get_buffer()
    {
        if (!buffer_check())
            return nullptr;
        return buffer_;
    }

    /**************************************************************/
    TInput::TInput()
    :fd_(-1)
    ,eof_(true)
    {}

    TInput::~TInput()
    {
        ::close(fd_);
    }

    void TInput::close()
    {
        ::close(fd_);
        eof_ = true;
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
            eof_ = false;
        }
        return true;
    }

    bool TInput::eof()
    {
        return eof_;
    }

    int TInput::read(char *dest, uint64_t len)
    {
        if (eof_)
            return 0;
        int r = ::read(fd_, dest, len);
        if (r <= 0)
            eof_ = true;
        return r;
    }

    /**************************************************************/

    ZInput::ZInput()
    :fd_(-1)
    ,eof_(true)
    {}

    ZInput::~ZInput()
    {
        ::close(fd_);
    }

    void ZInput::close()
    {
        ::close(fd_);
        eof_ = true;
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
            eof_ = false;
        }
        return true;
    }

    bool ZInput::eof()
    {
        return eof_;
    }

    int ZInput::read(char *dest, uint64_t len)
    {
        int ret;
        if (eof_)
            return 0;
        if (strm_.avail_in == 0)
        {
            int r = ::read(fd_, in_, sizeof(in_));
            if (r <= 0)
            {
                inflateEnd(&strm_);
                ::close(fd_);
                eof_ = true;
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
