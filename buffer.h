#ifndef BUFFER_H_
#define BUFFER_H_

#include <cstdint>

namespace logfind
{
    struct linebuf;

    static const int BUFSIZE = 1024*1024;

    class Buffer
    {
    public:
        Buffer();
        ~Buffer();
        uint64_t fileoffset();
        bool containsFileOffset(uint64_t offset);
        uint32_t getBufferOffsetFromFileOffset(uint64_t fileoff);
        uint32_t availableBytes();
        char *bufpos();
        void datasize(uint32_t n);
        bool isFull();
        bool eob();     // end of buffer
        void reset(uint64_t fileoffset);
        char getchar();
        bool readline(uint64_t fileoffset, linebuf&);
    private:
        char *buffer;       // buffer
        uint32_t bufsize;   // Size of the buffer
        uint32_t datalen;   // Amount of data in the buffer
        uint32_t curpos;    // Current read position into the buffer
        uint64_t offset;    // File offset of first char in buffer
        bool eob_;
    };
}

#endif
