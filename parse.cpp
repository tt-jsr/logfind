#include <stdexcept>
#include <sstream>
#include <iostream>
#include <assert.h>
#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "actions.h"
#include "pattern_actions.h"
#include "parse.h"

//#define DEBUG_LOG

namespace
{
    void ptoken(logfind::Token& t)
    {
        switch (t.tok)
        {
        case logfind::TOKEN_NL:
            std::cerr << "TOKEN_NL" << std::endl;
            break;
        case logfind::TOKEN_OPEN_BRACE:
            std::cerr << "TOKEN_OPEN_BRACE" << std::endl;
            break;
        case logfind::TOKEN_CLOSE_BRACE:
            std::cerr << "TOKEN_CLOSE_BRACE" << std::endl;
            break;
        case logfind::TOKEN_QUOTED_STRING:
            std::cerr << "TOKEN_QUOTED_STRING: " << t.str.c_str() << std::endl;
            break;
        case logfind::TOKEN_WORD:
            std::cerr << "TOKEN_WORD: " << t.str.c_str() << std::endl;
            break;
        case logfind::TOKEN_SEARCH_PATTERN:
            std::cerr << "TOKEN_SEARCH: " << t.str.c_str() << std::endl;
            break;
        case logfind::TOKEN_HASH:
            std::cerr << "TOKEN_HASH: " << t.str.c_str() << std::endl;
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
#ifdef DEBUG_LOG
        std::cerr << "tokenize complete\n" << std::endl;
#endif
        AhoFileContextPtr ptr = theApp->search();
        try
        {
            file_search(ptr);
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
                    c = fgetc(fp);
                    if (c == EOF)
                        return;
                }
                break;
            case '#':
                {
                    Token t;
                    t.tok = TOKEN_HASH;
                    t.lineno = lineno;
                    t.str = "#";
                    tokens_.push_back(t);
                    ++lineno;
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
                    ptoken(t);
#endif
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
#ifdef DEBUG_LOG
            ptoken(token);
#endif
            switch(token.tok)
            {
            case TOKEN_SEARCH_PATTERN:
                {
                    PatternActionsPtr pa = fp->add_match_text(token.str.c_str(), token.str.size());
                    pattern_action(pa);
                    assert(tokens_[tokIdx_].tok == TOKEN_CLOSE_BRACE);
                }
                break;
            case TOKEN_HASH:
                {
                    ++tokIdx_;
                    while (tokens_[tokIdx_].tok != TOKEN_NL)
                        ++tokIdx_;
                }
                break;
            case TOKEN_NL:
                break;
            default:
                std::stringstream strm;
                strm << "Error at line: " << token.lineno << ", expected '/'";
                throw std::runtime_error(strm.str());
            }
            ++tokIdx_;
        }
        fp->build_trie();
    }

    void Parse::pattern_action(PatternActionsPtr pa)
    {
#ifdef DEBUG_LOG
        std::cerr << "parse_action" << std::endl;
#endif
        assert(tokens_[tokIdx_].tok == TOKEN_SEARCH_PATTERN);
        ++tokIdx_;
        if (tokens_[tokIdx_].tok != TOKEN_OPEN_BRACE)
        {
            int lineno = tokens_[tokIdx_-1].lineno;
            std::stringstream strm;
            strm << "Error at line: " << tokens_[tokIdx_].lineno << ", expected '{'";
            throw std::runtime_error(strm.str());
        }
        ++tokIdx_;
        while (tokIdx_ < tokens_.size())
        {
            Token& token = tokens_[tokIdx_];
            switch(token.tok)
            {
            case TOKEN_CLOSE_BRACE:
#ifdef DEBUG_LOG
                ptoken(token);
#endif
                return;
            case TOKEN_QUOTED_STRING:
                {
#ifdef DEBUG_LOG
                    ptoken(token);
#endif
                    std::stringstream strm;
                    strm << "Error at line: " << token.lineno << ", unexpected " << token.str;
                    throw std::runtime_error(strm.str());
                }
                return;
            case TOKEN_OPEN_BRACE:
                {
#ifdef DEBUG_LOG
                    ptoken(token);
#endif
                    std::stringstream strm;
                    strm << "Error at line: " << token.lineno << ", unexpected '{'";
                    throw std::runtime_error(strm.str());
                }
                return;
            case TOKEN_SEARCH_PATTERN:
                {
#ifdef DEBUG_LOG
                    ptoken(token);
#endif
                    LineSearch *searchCmd = new LineSearch();
                    pa->add_action(searchCmd);
                    PatternActionsPtr pa = searchCmd->add_match_text(token.str.c_str(), token.str.size());
                    pattern_action(pa);
                    assert(tokens_[tokIdx_].tok == TOKEN_CLOSE_BRACE);
                    searchCmd->build_trie();
                    ++tokIdx_;
                }
                break;
            case TOKEN_WORD:
                {
                    Action *action = ActionFactory(token.str);
                    if (action == nullptr)
                    {
                        std::stringstream strm;
                        strm << "Error at line: " << token.lineno << ", unknown command " << token.str;
                        throw std::runtime_error(strm.str());
                    }
                    std::vector<std::string> args;
                    while (tokIdx_ < tokens_.size() && tokens_[tokIdx_].tok != TOKEN_NL)
                    {
                        args.push_back(tokens_[tokIdx_].str);
#ifdef DEBUG_LOG
                        ptoken(tokens_[tokIdx_]);
#endif
                        ++tokIdx_;
                    }
                    args.erase(args.begin());  // remove the command name
                    if (action->parse(args) == false)
                    {
                        std::stringstream strm;
                        strm << "Error at line: " << token.lineno << ", failed to parse command " << token.str;
                        throw std::runtime_error(strm.str());
                    }
                    pa->add_action(action);
                }
                break;
            case TOKEN_NL:
                ++tokIdx_;
                break;
            case TOKEN_HASH:
                {
                    ++tokIdx_;
                    while (tokens_[tokIdx_].tok != TOKEN_NL)
                        ++tokIdx_;
                }
                break;
            default:
#ifdef DEBUG_LOG
                std::cerr << "skipping: " << std::endl;
                ptoken(tokens_[tokIdx_]);
#endif
                ++tokIdx_;
                break;
            }
        }
        std::stringstream strm;
        strm << "Error: Unexpected end of file";
        throw std::runtime_error(strm.str());
    }
}
