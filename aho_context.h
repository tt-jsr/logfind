#ifndef AHO_CONTEXT_H_
#define AHO_CONTEXT_H_

#include <cstdint>
#include <unordered_map>
#include <string.h>
#include <vector>
#include <memory>
#include "file.h"

extern "C"
{
#include "ahocorasick.h"
}

namespace logfind
{

    class AhoContext;
    class AhoLineContext;
    class AhoFileContext;
    class PatternActions;

    using PatternActionsPtr = std::shared_ptr<PatternActions>;
    using AhoLineContextPtr = std::shared_ptr<AhoLineContext>;
    using AhoFileContextPtr = std::shared_ptr<AhoFileContext>;

    AhoLineContextPtr MakeAhoLineContext();
    AhoFileContextPtr MakeAhoFileContext();

    class AhoContext
    {
    public:
        AhoContext();
        ~AhoContext();
        PatternActionsPtr add_match_text(const char *p, uint32_t len);
        PatternActionsPtr add_match_text(const char *p);
        void build_trie();
        virtual char get() = 0;
        virtual bool readLine(uint64_t lineno, linebuf& l) = 0;
        void on_match(struct aho_match_t* m);
        void on_exit();
    protected:

        struct ahocorasick aho_;
        std::unordered_map<int, PatternActionsPtr> match_actions;
    };

    class AhoFileContext : public AhoContext
    {
    public:
        bool find(const char *fname);
    protected:
        ReadFile file;
    private:
        char get() override;
        bool readLine(uint64_t lineno, linebuf& line) override;
    };

    class AhoLineContext : public AhoContext
    {
    public:
        AhoLineContext();
        bool find(const char *p, uint32_t len, uint32_t lineno, uint64_t lineoff);
    protected:
        std::string line;
        size_t pos;
    private:
        char get() override;
        bool readLine(uint64_t lineno, linebuf& line) override;
    };
}

#endif

