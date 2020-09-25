#ifndef AHO_CONTEXT_H_
#define AHO_CONTEXT_H_

#include <cstdint>
#include <map>
#include <string.h>
#include <vector>
#include <memory>
#include "file.h"
#include "acism.h"
#include "linebuf.h"

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

    struct Match
    {
        Match()
        :line_offset_in_file(0)
        ,match_offset_in_file(0)
        ,match_offset_in_line(0)
        ,lineno(0)
        ,searchCtx(nullptr)
        {}

        linebuf matched_line;       // The line with the match
        std::string matched_text;   // The matched text
        F_OFFSET line_offset_in_file;              // The offset into the file for the line
        F_OFFSET match_offset_in_file;             // The Offset into the file for the match
        L_OFFSET match_offset_in_line;     // The offset into the line for the match
        int lineno;                 // The line number that matched
        AhoContext *searchCtx;
    };

    class AhoContext
    {
    public:
        AhoContext();
        virtual ~AhoContext();
        PatternActionsPtr add_match_text(const char *p, uint32_t len);
        PatternActionsPtr add_match_text(const char *p);
        PatternActionsPtr getPatternActions(const char *);
        int on_match(int strnum, B_OFFSET textpos);
        virtual void on_line(B_OFFSET linepos) {};
        virtual void matchData(Match& m, B_OFFSET matchpos) = 0;
        void on_exit();
    protected:
        std::unordered_map<int, PatternActionsPtr> match_actions_;
        std::unordered_map<std::string, int> match_str_actions_;
        std::vector<std::string> patterns_;
        ACISM *acism_;
        MEMREF *pattv_;
        int npatts_;
    };

    class AhoFileContext : public AhoContext
    {
    public:
        ~AhoFileContext() {}
        bool find(const char *fname);
        std::string filename() {return file.filename();}
    protected:
        ReadFile file;
    private:
        void matchData(Match& m, B_OFFSET matchpos) override;
        void on_line(B_OFFSET textpos);
        F_OFFSET current_line_offset_;
        uint32_t current_lineno_;
        Buffer *current_buffer_;
        B_OFFSET current_block_offset_;
        std::map<uint64_t, F_OFFSET> lines_; // lineno => fileoffset
    };

    class AhoLineContext : public AhoContext
    {
    public:
        AhoLineContext();
        ~AhoLineContext() {}
        //bool find(const char *p, uint32_t len, uint32_t lineno, uint64_t lineoff);
        void matchData(Match& m, B_OFFSET matchpos) override;
    protected:
        std::string matched_line_;
        uint64_t matched_lineno_;

    private:
        friend class LineSearch;
    };
}

#endif

