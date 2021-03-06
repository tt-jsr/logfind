#ifndef FILE_H_
#define FILE_H_
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>
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
        virtual bool eof() = 0;
    };

    // Test input
    class TInput : public Input
    {
    public:
        TInput();
        ~TInput();
        // '-' for stdin
        bool open(const char *);
        void close() override;
        int read(char *dest, uint64_t len) override;
        bool eof() override;
    private:
        int fd_;
        bool eof_;
        std::string filename_;
    };

    // compressed .gz inout
    class ZInput : public Input
    {
    public:
        ZInput();
        ~ZInput();
        void close() override;
        // '-' for stdin
        bool open(const char *);
        int read(char *dest, uint64_t len) override;
        bool eof() override;
    private:
        static const int CHUNK = 1024*1024;
        int fd_;
        bool eof_;
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
        //char get();
        int read(char *buf, int len); // untested
        bool readLine(uint64_t lineoff, linebuf&);
        bool eof();
        std::string filename();

        // Read from the buffer directly
        Buffer *get_buffer();

    protected:
        std::string filename_;
        bool buffer_check();
    private:
        void reset();
        void read_ahead_thread_();
    private:
        Input *pInput_;
        Buffer *buffer_;
        LRUCache cache_;
        std::mutex mtx_;
        std::condition_variable cv_;
        bool bRun_;
        int read_ahead_;
        uint64_t next_read_ahead_buffer_key_;
        std::thread thrd_;
    };
}
#endif
