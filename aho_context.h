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
    class PatternActions;

    using PatternActionsPtr = std::shared_ptr<PatternActions>;

    class PatternActions
    {
    public:
        void on_match(AhoContext *ctx, struct aho_match_t* m);
        int add_match_text(const char *p, uint32_t len, PatternActionsPtr actions);
        int add_match_text(const char *p, PatternActionsPtr actions);
        void before(uint8_t n);
        void after(uint8_t n);
        void file(const char *name);
        void search(std::shared_ptr<AhoLineContext>);
        void print();
    private:
        void printlines(AhoContext *ctx, struct aho_match_t* m, int32_t start, uint32_t end);
        std::shared_ptr<AhoLineContext> pCtx_;
        uint8_t before_;
        uint8_t after_;
        std::string outfile_;
        std::vector<std::string> commands_;
    };

    PatternActionsPtr MakePatternActions();


    class AhoContext
    {
    public:
        AhoContext();
        ~AhoContext();
        int add_match_text(const char *p, uint32_t len, PatternActionsPtr actions);
        int add_match_text(const char *p, PatternActionsPtr actions);
        void build_trie();
        virtual char get() = 0;
        virtual bool readLine(uint64_t lineno, linebuf& l) = 0;
        void on_match(struct aho_match_t* m);
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
        bool find(const char *p, uint32_t len);
    protected:
        std::string line;
        size_t pos;
    private:
        char get() override;
        bool readLine(uint64_t lineno, linebuf& line) override;
    };
}

#endif

