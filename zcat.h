#ifndef ZCAT_H_
#define ZCAT_H_

#include "zlib.h"

namespace logfind
{
    class ZCat
    {
    public:
        void read();
    private:
        z_stream strm_;
        int fd_;
        char in[1024*100];
    };
}

#endif

