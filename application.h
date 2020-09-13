#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <memory>
#include <unordered_map>

namespace logfind
{
    class AhoFileContext;
    struct linebuf;
    class PatternActions;
    using PatternActionsPtr = std::shared_ptr<PatternActions>;
    using AhoFileContextPtr = std::shared_ptr<AhoFileContext>;

    class Application
    {
    public:
        Application();

        void on_exit();
        void exit();
        std::string filename();
        void NamedPatternAction(const char *name, PatternActionsPtr);
        PatternActionsPtr GetNamedPattern(const char *);
        int file(const char *, bool append);
        void free(linebuf& lb);
        bool alloc(linebuf& lb);

        bool is_exit();
        AhoFileContextPtr search();
    private:
        AhoFileContextPtr pCtx_;
        std::unordered_map<std::string, PatternActionsPtr> named_patterns_;
        std::unordered_map<std::string, int> files_;    // filepath=>fd
        bool exit_flag;
    };

    extern Application *theApp;
}

#endif
