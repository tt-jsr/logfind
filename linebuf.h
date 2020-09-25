
#ifndef LINEBUF_H_
#define LINEBUF_H_

namespace logfind 
{
    enum
    {
        LINEBUF_INVALID,
        LINEBUF_NONE,
        LINEBUF_FREE,
        LINEBUF_POOL
    };

    struct linebuf
    {
        linebuf()
        :buf(nullptr), len(0), bufsize(0), flags(LINEBUF_NONE)
        {}
        char *buf;
        uint32_t len;
        uint32_t bufsize;
        uint8_t flags;
    };
}

#endif
