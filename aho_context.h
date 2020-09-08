#ifndef AHO_CONTEXT_H_
#define AHO_CONTEXT_H_

#include <cstdint>
#include <unordered_map>
#include <string.h>
#include "file.h"

extern "C"
{
#include "ahocorasick.h"
}

namespace logfind
{

    class AhoContext;

    class PatternActions
    {
    public:
        void on_match(AhoContext *ctx, struct aho_match_t* m);
    };

    class AhoContext
    {
    public:
        AhoContext();
        ~AhoContext();
        int add_match_text(const char *p, uint32_t len, PatternActions& actions);
        int add_match_text(const char *p, PatternActions& actions);
        void build_trie();
        virtual char get() = 0;
        virtual void readLine(uint64_t lineno, char *buf, uint32_t len) = 0;
        void on_match(struct aho_match_t* m);
    protected:

        struct ahocorasick aho_;
        std::unordered_map<int, PatternActions> match_actions;
    };

    class AhoFileContext : public AhoContext
    {
    public:
        bool find(const char *fname);
    protected:
        ReadFile file;
    private:
        char get() override;
        void readLine(uint64_t lineno, char *buf, uint32_t len) override;
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
        void readLine(uint64_t lineno, char *buf, uint32_t len) override;
    };
}

#endif

