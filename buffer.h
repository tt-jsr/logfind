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

        // Does this buffer contain this file offset?
        bool containsFileOffset(uint64_t offset);

        // Given a file offset, get the offset into this buffer
        // Return (uint64_t)-1 if invalid offset.
        uint32_t getBufferOffsetFromFileOffset(uint64_t fileoff);

        /*******************************************
         * writing into buffer
         */
        // Available bytes for writing
        uint32_t availableWriteBytes();
        void incrementAvailableReadBytes(uint32_t n);

        // Get a pointer to write to, with the available number of bytes
        char *writePos();

        // Is buffer full?, same as availableWriteBytes() == 0
        bool isFull();

        /******************************************
         * Reading from buffer
         */
        // Available bytes for reading
        uint32_t availableReadBytes();

        // current read position
        char *readPos();

        // get the character at the current read position
        char getchar();

        // Are we at the end of buffer for reading?, same as availableReadBytes() == 0
        bool eob();     // end of buffer

        // read a line, does not alter the current read position
        bool readline(uint64_t fileoffset, linebuf&);

        void reset(uint64_t fileoffset);
    private:
        char *buffer_;       // buffer
        uint32_t bufsize_;   // Size of the buffer
        uint32_t datalen_;   // Amount of data in the buffer
        uint32_t readpos_;    // Current read position into the buffer
        uint64_t offset_;    // File offset of first char in buffer
    };
}

#endif
