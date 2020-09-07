#include "file.h"
#include <sys/types.h>
#include <unistd.h>

namespace loggrep
{
    const int bufsize = 1024*1024;

    ReadFile::ReadFile()
    :fd_(-1)
    ,offset_(0)
    ,buffer_(nullptr)
    ,bufsize_(0)
    ,datalen_(0)
    ,curpos_(0)
    {
        buffer_ = new char[bufsize];
        bufsize_ = bufsize;
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
            fd_ = open(fname, O_RDONLY);
            if (fd_ < 0)
                return false;
        }
        read_();
    }

    void ReadFile::read_()
    {
        if (fd_ > 2)
            offset_ = lseek(fd_, 0, SEEK_SET);
        int r = read(fd_, buffer_, bufsize_);
        if (r >= 0)
            datalen_ = r;
        curpos_ = 0;
    }

    char ReadFile::get()
    {
        if (datalen_ == 0)
            return 0;
        if (curpos_ == datalen_)
        {
            read_();
        }
        if (datalen_ == 0)
            return 0;
        return buffer_[curpos_++];
    }
}
