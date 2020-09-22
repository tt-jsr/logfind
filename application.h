#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <memory>
#include <unordered_map>

namespace logfind
{
    class AhoFileContext;
    class AhoLineContext;
    struct linebuf;
    class PatternActions;
    using PatternActionsPtr = std::shared_ptr<PatternActions>;
    using AhoFileContextPtr = std::shared_ptr<AhoFileContext>;
    using AhoLineContextPtr = std::shared_ptr<AhoLineContext>;

    struct Data
    {
        virtual std::string tostr() = 0;
    };

    struct StringData : public Data
    {
        std::string str;
        std::string tostr() {return str;}
    };

    class Application
    {
    public:
        Application();

        void on_exit();
        void on_start();
        void on_file_start(const char *);
        void on_file_end(const char *);
        void exit();
        std::string filename();
        AhoLineContextPtr GetNamedContext(const std::string&);
        int file(const char *, bool append);
        void free(linebuf& lb);
        bool alloc(linebuf& lb);

        Data *getData(const std::string& name);
        void setData(const std::string& name, Data *);
        bool is_exit();
        AhoFileContextPtr search();
    private:
        AhoFileContextPtr pCtx_;
        AhoLineContextPtr pNamedPatternCtx_;
        std::unordered_map<std::string, int> files_;    // filepath=>fd
        std::unordered_map<std::string, Data *> data_;    // filepath=>fd
        bool exit_flag;
    };

    extern Application *theApp;
}

#endif
