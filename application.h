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

    class Application
    {
    public:
        Application();

        void exit();
        void NamedPatternAction(const char *name, PatternActionsPtr);
        PatternActionsPtr GetNamedPattern(const char *);
        int file(const char *, bool append);
        void free(linebuf& lb);
        bool alloc(linebuf& lb);

        std::unique_ptr<AhoFileContext> pCtx;
        bool is_exit();
    private:
        std::unordered_map<std::string, PatternActionsPtr> named_patterns_;
        std::unordered_map<std::string, int> files_;
        bool exit_flag;
    };

    extern Application *theApp;
}

#endif
