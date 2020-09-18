#ifndef FILE_H_
#define FILE_H_
#include <unordered_map>
#include "zlib.h"

namespace logfind
{
    class Buffer;
    struct linebuf;

    class Input
    {
    public:
        virtual ~Input() {};
        
        virtual bool open(const char *) = 0;
        virtual void close() = 0;
        virtual int read(char *dest, uint64_t len) = 0;
    };

    class TInput : public Input
    {
    public:
        ~TInput();
        // '-' for stdin
        bool open(const char *);
        void close() override;
        int read(char *dest, uint64_t len) override;
    private:
        int fd_;
        std::string filename_;
    };

    class ZInput : public Input
    {
    public:
        ~ZInput();
        void close() override;
        // '-' for stdin
        bool open(const char *);
        int read(char *dest, uint64_t len) override;
    private:
        static const int CHUNK = 1024*256;
        int fd_;
        std::string filename_;
        z_stream strm_;
        char in_[CHUNK];
    };

    class ReadFile
    {
    public:
        ReadFile();
        ~ReadFile();

        // '-' for stdin
        bool open(const char *);
        char get();
        bool readLine(uint64_t lineoff, linebuf&);
        bool eof();
        std::string filename();
    protected:
        std::string filename_;
        int read_();
    private:
        Buffer *getBufferFromFileOffset(uint64_t offset);
        void reset();
    private:
        Input *pInput_;
        uint64_t offset_;
        uint64_t lineno_;
        std::unordered_map<uint64_t, uint64_t> lines_; // lineno => fileoffset
        Buffer *buffer_;
        LRUCache cache_;
        bool eof_;
    };
}
#endif
