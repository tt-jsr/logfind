#ifndef FILE_H_
#define FILE_H_

#include <string>

namespace loggrep
{
    class ReadFile
    {
    public:
        ReadFile();
        ~ReadFile();
        // null for stdin
        bool open(const char *);
        bool get();
    private:
        void read_();
    private:
        int fd_;
        uint64_t offset_;
        char *buffer_;
        uint32_t bufsize_;
        uint32_t datalen_;
        uint32_t curpos_;
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
