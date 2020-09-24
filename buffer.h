#ifndef BUFFER_H_
#define BUFFER_H_

#include <cstdint>
#include <cstddef>

namespace logfind
{
    struct linebuf;

    static const int BUFSIZE = 1024*1024;

    using F_OFFSET = uint64_t;      // file offset
    using B_OFFSET = uint64_t;      // buffer offset, offset from begining of buffer
    using L_OFFSET = uint64_t;      // line offset, offset from begining of line

    /* Given a file offset, returns the buffer key
     * for the lru_cache
     */
    inline F_OFFSET fileOffsetToBufferKey(F_OFFSET off) { return off/BUFSIZE * BUFSIZE;}

    class Buffer
    {
    public:
        Buffer();
        ~Buffer();

        // Buffer file offset, this is also the key into the LRU cache
        F_OFFSET fileoffset();

        // Does this buffer contain this file offset?
        bool containsFileOffset(F_OFFSET offset);

        // Given a file offset, get the offset into this buffer
        // Return (uint64_t)-1 if invalid offset.
        B_OFFSET getBufferOffsetFromFileOffset(F_OFFSET fileoff);

        /*******************************************
         * writing into buffer
         */
        // Available bytes for writing
        size_t availableWriteBytes();
        void incrementAvailableReadBytes(size_t n);

        // Get a pointer to write to, with the available number of bytes
        char *writePos();

        // Is buffer full?, same as availableWriteBytes() == 0
        bool isFull();

        /******************************************
         * Reading from buffer
         */
        // Available bytes for reading
        size_t availableReadBytes();
        void incrementReadPosition(size_t);

        // Set the readPos via a pointer into the buffer
        bool setReadPos(const char *p);

        // current read position
        char *readPos();

        // get the character at the current read position
        char getchar();

        // Are we at the end of buffer for reading?, same as availableReadBytes() == 0
        bool eob();     // end of buffer

        // read a line, does not alter the current read position
        bool readline(F_OFFSET fileoffset, linebuf&);

        void reset(F_OFFSET fileoffset);
    private:
        char *buffer_;       // buffer
        size_t bufsize_;   // Size of the buffer
        size_t datalen_;   // Amount of data in the buffer
        B_OFFSET readpos_;   // Current read position into the buffer
        F_OFFSET offset_;    // File offset of first char in buffer
    };
}

#endif
