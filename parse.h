#ifndef PARSE_H_
#define PARSE_H_

#include <vector>
#include <stdio.h>

namespace logfind
{
    enum
    {
        TOKEN_OPEN_BRACE,
        TOKEN_CLOSE_BRACE,
        TOKEN_WORD,
        TOKEN_SEARCH_PATTERN,
        TOKEN_QUOTED_STRING,
        TOKEN_NL
    };

    struct Token
    {
        int tok;
        std::string str;
        int lineno;
    };

    class Parse
    {
    public:
        Parse();
        ~Parse();

        bool parse(const char *file);
    private:
        void tokenize(FILE *);
        void file_search(AhoFileContextPtr fp);
        void pattern_action(PatternActionsPtr pa);

        std::vector<Token> tokens_;
        size_t tokIdx_;
    };
}

#endif
