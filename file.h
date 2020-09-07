#ifndef FILE_H_
#define FILE_H_

#include <string>
#include <unordered_map>

namespace logfind
{
    struct Buffer
    {
        char *buffer;       // buffer
        uint32_t bufsize;   // Size of the buffer
        uint32_t datalen;   // Amount of data read into the buffer
        uint32_t curpos;    // Current read position into the buffer
        uint64_t offset;    // File offset of first char in buffer
    };

    class ReadFile
    {
    public:
        ReadFile();
        ~ReadFile();
        // null for stdin
        bool open(const char *);
        char get();
        void readLine(uint64_t lineoff, char *linebuf, uint32_t size);
    private:
        void read_();
        uint32_t bytesToRead();
        Buffer *getBufferFromOffset(uint64_t offset);
    private:
        int fd_;
        uint64_t offset_;
        uint64_t lineno_;
        std::unordered_map<uint64_t, uint64_t> lines_; // lineno => fileoffset
        Buffer *prev_;
        Buffer *buffer_;
    };
/*
    class WriteFile
    {
    public:
        WriteFile();
        ~WriteFile();
        // null for stdout
        bool open(const char *, bool truncate);
        bool write(const std::string&);
        bool write(const char *, uint32_t len);
        bool writenl();
    private:
        int fd_;
        char *buffer_;
        uint32_t bufsize_;
        char *pos_;
        char *endbuf_;
    };
    */
}
#endif
