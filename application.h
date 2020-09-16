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

        bool is_exit();
        AhoFileContextPtr search();
    private:
        AhoFileContextPtr pCtx_;
        AhoLineContextPtr pNamedPatternCtx_;
        std::unordered_map<std::string, int> files_;    // filepath=>fd
        bool exit_flag;
    };

    extern Application *theApp;
}

#endif
