#ifndef FILE_H_
#define FILE_H_


namespace logfind
{
    class Buffer;
    struct linebuf;

    class ReadFile
    {
    public:
        ReadFile();
        ~ReadFile();
        // null for stdin
        bool open(const char *);
        char get();
        bool readLine(uint64_t lineoff, linebuf&);
        bool eof();
    private:
        int read_();
        Buffer *getBufferFromFileOffset(uint64_t offset);
    private:
        int fd_;
        uint64_t offset_;
        uint64_t lineno_;
        std::unordered_map<uint64_t, uint64_t> lines_; // lineno => fileoffset
        Buffer *buffer_;
        LRUCache cache_;
        bool eof_;
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
