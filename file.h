#ifndef FILE_H_
#define FILE_H_


namespace logfind
{
    class Buffer;
    struct linebuf;

    class Input
    {
    public:
        virtual ~Input() {};
        
        virtual void close() = 0;
        virtual int read(char *dest, uint64_t len) = 0;
        virtual std::string filename() = 0;
    };

    class TFile : public Input
    {
    public:
        ~TFile();
        // null for stdin
        bool open(const char *);
        void close() override;
        int read(char *dest, uint64_t len) override;
        std::string filename() override;
    private:
        int fd_;
        std::string filename_;
    };

    class ZFile : public Input
    {
    public:
        ~ZFile();
        void close() override;
        // null for stdin
        bool open(const char *);
        int read(char *dest, uint64_t len) override;
        std::string filename() override;
    private:
        int fd_;
        std::string filename_;
    };

    class ReadFile
    {
    public:
        ReadFile();
        ~ReadFile();

        // input must be ready for reading when attached
        void attach(Input *);
        char get();
        bool readLine(uint64_t lineoff, linebuf&);
        bool eof();
        std::string filename();
    protected:
        std::string filename_;
        int read_();
    private:
        Buffer *getBufferFromFileOffset(uint64_t offset);
    private:
        Input *pInput;
        uint64_t offset_;
        uint64_t lineno_;
        std::unordered_map<uint64_t, uint64_t> lines_; // lineno => fileoffset
        Buffer *buffer_;
        LRUCache cache_;
        bool eof_;
    };
}
#endif
