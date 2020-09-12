#include <sstream>
#include <unistd.h>
#include <limits>
#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "builtins.h"
#include "pattern_actions.h"

namespace logfind
{
    bool After::parse(const std::vector<std::string>& args)
    {
    }

    void After::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        linebuf lb;
        uint32_t start = lineno;
        uint32_t end = lineno+lines_;

        for (uint32_t l = start; l < end; ++l)
        {
            pCtx_->readLine(l, lb);

            dprintf(fd, "%s\n", lb.buf);
            theApp->free(lb);
        }
    }

    /*********************************************************************/

    bool Before::parse(const std::vector<std::string>& args)
    {
    }

    void Before::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        linebuf lb;
        uint32_t start = lineno-lines_;
        if (lines_ > lineno)
            start = 0;
        uint32_t end = lineno;

        for (uint32_t l = start; l < end; ++l)
        {
            pCtx_->readLine(l, lb);

            dprintf(fd, "%s\n", lb.buf);
            theApp->free(lb);
        }
    }

    /*********************************************************************/

    // parse the ${line:n,m} format
    bool Print::parse_line_fmt(const char *&p)
    {
        switch (*p)
        {
        case '}':
            ++p;
            break;
        case ':':
            ++p;
            char sbuf[32], ebuf[32];
            char *dest = sbuf;
            while(*p && *p != '}')
            {
               switch(*p)
               {
                case ',':
                    *dest = '\0';
                    dest = ebuf;
                    break;
                default:
                    if ((*p >= '0' && *p <= '9') || *p == '-')
                    {
                        *dest = *p;
                        ++dest;
                    }
               }
               ++p;
            }
            *dest = '\0';
            if (sbuf[0] == '\0')
                line_start_ = 0;
            else
                line_start_ = strtol(sbuf, nullptr, 10);
            if (ebuf[0] == '\0')
                line_end_ = std::numeric_limits<int>::max();
            else
                line_end_ = strtol(ebuf, nullptr, 10);
        }
        return true;
    }

    Print::Print()
    :line_start_(0)
    ,line_end_(std::numeric_limits<int>::max())
    ,match_delim_(0)
    {}

    bool Print::parse(const std::vector<std::string>& args)
    {
        if (args.size() > 1)
        {
            const char *p = args[1].c_str();
            std::stringstream strm;
            while(*p)
            {
                if (strncmp(p, "${lineno}", 9) == 0)
                {
                    strm << "${1}";
                    p += 9;
                }
                else if (strncmp(p, "${offset}", 9) == 0)
                {
                    strm << "${2}";
                    p += 9;
                }
                else if (strncmp(p, "${line", 6) == 0)
                {
                    strm << "${3}";
                    p += 6;
                    if (parse_line_fmt(p) == false)
                        return false;
                }
                else if (strncmp(p, "${match/", 8) == 0)
                {
                    strm << "${4}";
                    p += 8;
                    match_delim_ = *p;
                    ++p;
                }
                else
                {
                    strm << *p;
                    ++p;
                }
            }
            format_ = strm.str();
        }
        return true;
    }

    void Print::substitute(const char *&p, int fd, uint32_t lineno, linebuf& matchingline)
    {
        if (strncmp(p, "${1}", 4) == 0)  //${lineno}
        {
            char buf[32];
            sprintf (buf, "%d", lineno);
            write(fd, buf, strlen(buf));
            p += 4;
            return;
        }
        if (strncmp(p, "${2}", 4) == 0)     // ${offset}
        {
            char buf[32];
            sprintf (buf, "%d", aho_match_->line_match_pos);
            write(fd, buf, strlen(buf));
            p += 4;
            return;
        }
        if (strncmp(p, "${3}", 4) == 0)     // ${line}
        {
            int start(line_start_), end(line_end_);
            if (start > (int)matchingline.len)
            {
                start = (int)matchingline.len;
            }
            if (start < 0)
            {
                start = (int)matchingline.len + start;
                if (start < 0)
                    start = 0;
            }
            if (end > (int)matchingline.len)
                end = (int)matchingline.len;
            if (end < 0)
            {
                end = (int)matchingline.len + end;
                if (end < 0)
                    end = 0;
            }
            if (start > end)
                start = end;

            int len = end-start;
            write(fd, matchingline.buf+start, len);
            p += 4;
            return;
        }

        if (strncmp(p, "${4}", 4) == 0)     // ${match}
        {
            char *src = matchingline.buf+aho_match_->line_match_pos;
            write(fd, src, aho_match_->len);
            src += aho_match_->len;
            while (*src != match_delim_ && src < matchingline.buf+matchingline.len)
            {
                write(fd, src, 1);
                ++src;
            }
            p += 4;
        }
    }

    void Print::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        if (format_.empty())
        {
            dprintf(fd, "%s\n", matchingline.buf);
            return;
        }
        const char * p = format_.c_str();
        while (*p)
        {
            if (*p == '$' && *(p+1) == '{')
            {
                substitute(p, fd, lineno, matchingline);
                ++p;
            }
            else if (*p == '\\' && *(p+1) == 'n')
            {
                write(fd, "\n", 1);
                p += 2;
            }
            else
            {
                write(fd, p, 1);
                ++p;
            }
        }
    }

    /*********************************************************************/

    LineSearch::LineSearch()
    {
        lineSearch_ = MakeAhoLineContext();
    }

    bool LineSearch::parse(const std::vector<std::string>& args)
    {
    }

    void LineSearch::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        lineSearch_->find(matchingline.buf, matchingline.len, aho_match_->lineno, aho_match_->line_position_in_file);
    }

    PatternActionsPtr LineSearch::add_match_text(const char *p, uint32_t len)
    {
        return lineSearch_->add_match_text(p, len);
    }

    PatternActionsPtr LineSearch::add_match_text(const char *p)
    {
        return lineSearch_->add_match_text(p);
    }

    void LineSearch::build_trie()
    {
        lineSearch_->build_trie();
    }

    /*********************************************************************/

    bool Exit::parse(const std::vector<std::string>& args)
    {
    }

    void Exit::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        theApp->exit();
    }

    /*********************************************************************/

    bool NamedPatternActions::parse(const std::vector<std::string>& args)
    {
    }

    void NamedPatternActions::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        PatternActionsPtr pa = theApp->GetNamedPattern(name_.c_str());
        if (pa)
        {
            pa->on_match(pCtx_, aho_match_);
        }
    }

    /*********************************************************************/

    File::File()
    :append_(false)
    {}

    bool File::parse(const std::vector<std::string>& args)
    {
    }

    void File::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        pattern_actions_->fd_ = theApp->file(name_.c_str(), append_);
    }

    /*********************************************************************/

    Builtin *BuiltinFactory(const std::string& name)
    {
        if (name == "print")
            return new Print();
        else if (name == "exit")
            return new Exit();
        else if (name == "file")
            return new File();
        //else if (name == "after")
            //return new After();
        //else if (name == "before")
            //return new Before();
        else
            return nullptr;
    }
}

