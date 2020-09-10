#include <stdexcept>
#include <sstream>
#include <assert.h>
#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "builtins.h"
#include "pattern_actions.h"
#include "parse.h"

namespace
{
    void ptoken(logfind::Token& t)
    {
        switch (t.tok)
        {
        case logfind::TOKEN_NL:
            printf ("TOKEN_NL\n");
            break;
        case logfind::TOKEN_OPEN_BRACE:
            printf ("TOKEN_OPEN_BRACE\n");
            break;
        case logfind::TOKEN_CLOSE_BRACE:
            printf ("TOKEN_CLOSE_BRACE\n");
            break;
        case logfind::TOKEN_QUOTED_STRING:
            printf ("TOKEN_QUOTED_STRING: %s\n", t.str.c_str());
            break;
        case logfind::TOKEN_WORD:
            printf ("TOKEN_WORD: %s\n", t.str.c_str());
            break;
        case logfind::TOKEN_SEARCH_PATTERN:
            printf ("TOKEN_SEARCH: %s\n", t.str.c_str());
            break;
        }
    }
}

namespace logfind
{
    Parse::Parse()
    {

    }

    Parse::~Parse()
    {

    }

    bool Parse::parse(const char *fname)
    {
        FILE *fp = fopen(fname, "r");
        if (fp == nullptr)
            return false;
        tokenize(fp);
        fclose(fp);

        printf ("tokenize complete\n");
        AhoFileContextPtr ptr = theApp->search();
        try
        {
            file_search(ptr);
        }
        catch (std::exception& e)
        {
            printf ("%s\n", e.what());
            return false;
        }
        return true;
    }

    void Parse::tokenize(FILE *fp)
    {
        tokIdx_ = 0;
        int lineno = 0;
        char c = fgetc(fp);
        if (c == EOF)
            return;
        while (true)
        {
            switch (c)
            {
            case '\n':
                {
                    Token t;
                    t.tok = TOKEN_NL;
                    t.lineno = lineno;
                    t.str = "\n";
                    tokens_.push_back(t);
                    ++lineno;
                    ptoken(t);
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case ' ':
                {
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '\t':
                {
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '/':
                {
                    Token t;
                    c = fgetc(fp);
                    while (c != '/')
                    {
                        t.str.push_back(c);
                        c = fgetc(fp);
                        if (c == EOF)
                        {
                            std::stringstream strm;
                            strm << "Unexpected end of file";
                            throw std::runtime_error(strm.str());
                        }
                    }
                    t.tok = TOKEN_SEARCH_PATTERN;
                    t.lineno = lineno;
                    tokens_.push_back(t);
                    ptoken(t);
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '{':
                {
                    Token t;
                    t.tok = TOKEN_OPEN_BRACE;
                    t.str = "{";
                    t.lineno = lineno;
                    tokens_.push_back(t);
                    ptoken(t);
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '}':
                {
                    Token t;
                    t.tok = TOKEN_CLOSE_BRACE;
                    t.str = "}";
                    t.lineno = lineno;
                    tokens_.push_back(t);
                    ptoken(t);
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '"':
                {
                    Token t;
                    c = fgetc(fp);
                    while (c != '"')
                    {
                        t.str.push_back(c);
                        c = fgetc(fp);
                        if (c == EOF)
                        {
                            std::stringstream strm;
                            strm << "Unexpected end of file";
                            throw std::runtime_error(strm.str());
                        }
                    }
                    t.tok = TOKEN_QUOTED_STRING;
                    t.lineno = lineno;
                    tokens_.push_back(t);
                    ptoken(t);
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            default:
                {
                    Token t;
                    while (c != ' ' && c != '\t' && c != '\n')
                    {
                        t.str.push_back(c);
                        c = fgetc(fp);
                        if (c == EOF)
                        {
                            std::stringstream strm;
                            strm << "Unexpected end of file";
                            throw std::runtime_error(strm.str());
                        }
                    }
                    t.tok = TOKEN_WORD;
                    t.lineno = lineno;
                    tokens_.push_back(t);
                    ptoken(t);
                }
            }
        }
    }

    void Parse::file_search(AhoFileContextPtr fp)
    {
        tokIdx_ = 0;
        while (tokIdx_ < tokens_.size())
        {
            Token& token = tokens_[tokIdx_];
            //ptoken(token);
            switch(token.tok)
            {
            case TOKEN_SEARCH_PATTERN:
                {
                    PatternActionsPtr pa = fp->add_match_text(token.str.c_str(), token.str.size());
                    pattern_action(pa);
                    assert(tokens_[tokIdx_].tok == TOKEN_CLOSE_BRACE);
                }
                break;
            case TOKEN_NL:
                break;
            default:
                std::stringstream strm;
                strm << "Error: " << token.lineno << ", expected '/'";
                throw std::runtime_error(strm.str());
            }
            ++tokIdx_;
        }
        fp->build_trie();
    }

    void Parse::pattern_action(PatternActionsPtr pa)
    {
        printf ("parse_action\n");
        assert(tokens_[tokIdx_].tok == TOKEN_SEARCH_PATTERN);
        ++tokIdx_;
        if (tokens_[tokIdx_].tok != TOKEN_OPEN_BRACE)
        {
            int lineno = tokens_[tokIdx_-1].lineno;
            std::stringstream strm;
            strm << "Error: " << tokens_[tokIdx_].lineno << ", expected '{'";
            throw std::runtime_error(strm.str());
        }
        ++tokIdx_;
        while (tokIdx_ < tokens_.size())
        {
            Token& token = tokens_[tokIdx_];
            switch(token.tok)
            {
            case TOKEN_CLOSE_BRACE:
                ptoken(token);
                return;
            case TOKEN_QUOTED_STRING:
                {
                    ptoken(token);
                    std::stringstream strm;
                    strm << "Error: " << token.lineno << ", unexpected " << token.str;
                    throw std::runtime_error(strm.str());
                }
                return;
            case TOKEN_OPEN_BRACE:
                {
                    ptoken(token);
                    std::stringstream strm;
                    strm << "Error: " << token.lineno << ", unexpected '{'";
                    throw std::runtime_error(strm.str());
                }
                return;
            case TOKEN_SEARCH_PATTERN:
                {
                    ptoken(token);
                    LineSearch *searchCmd = new LineSearch();
                    pa->add_command(searchCmd);
                    PatternActionsPtr pa = searchCmd->add_match_text(token.str.c_str(), token.str.size());
                    pattern_action(pa);
                    assert(tokens_[tokIdx_].tok == TOKEN_CLOSE_BRACE);
                    searchCmd->build_trie();
                    ++tokIdx_;
                }
                break;
            case TOKEN_WORD:
                {
                    Builtin *bi = BuiltinFactory(token.str);
                    if (bi == nullptr)
                    {
                        std::stringstream strm;
                        strm << "Error: " << token.lineno << ", unknown command " << token.str;
                        throw std::runtime_error(strm.str());
                    }
                    std::stringstream args;
                    while (tokIdx_ < tokens_.size() && tokens_[tokIdx_].tok != TOKEN_NL)
                    {
                        args << " ";
                        if (tokens_[tokIdx_].tok == TOKEN_QUOTED_STRING)
                            args << '"';
                        args << tokens_[tokIdx_].str;
                        if (tokens_[tokIdx_].tok == TOKEN_QUOTED_STRING)
                            args << '"';
                        ptoken(tokens_[tokIdx_]);
                        ++tokIdx_;
                    }
                    bi->parse(args.str());
                    pa->add_command(bi);
                }
                break;
            case TOKEN_NL:
                ++tokIdx_;
                break;
            default:
                printf ("skipping: ");
                ptoken(tokens_[tokIdx_]);
                ++tokIdx_;
                break;
            }
        }
        printf("eot\n");
        std::stringstream strm;
        strm << "Error: Unexpected end of file";
        throw std::runtime_error(strm.str());
    }
}
