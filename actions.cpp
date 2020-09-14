#include <sstream>
#include <unistd.h>
#include <limits>
#include <assert.h>
#include <iostream>
#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "actions.h"
#include "pattern_actions.h"
#include "utilities.h"

namespace logfind
{
    uint32_t Action::get_match_pos()
    {
        return aho_match_->line_match_pos;
    }

    uint32_t Action::get_match_len()
    {
        return aho_match_->len;
    }

    /*********************************************************************/

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
    ,match_additional_(0)
    {}

    bool Print::parse(const std::vector<std::string>& args)
    {
        if (args.size() > 0)
        {
            const char *p = args[0].c_str();
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
                    assert(*p == '}');
                    ++p;  // pass the '}'
                }
                else if (strncmp(p, "${match", 7) == 0)
                {
                    strm << "${4}";
                    p += 7;
                    if (*p == '/')
                    {
                        ++p; // get pass the '/'
                        match_delim_ = *p;
                        ++p;    // get pass the delimiter
                    }
                    else if (*p == '+')
                    {
                        ++p; // get pass the '+'
                        char *e;
                        match_additional_ = std::strtol(p, &e, 10);
                        p = e;
                    }
                    assert(*p == '}');
                    ++p;
                }
                else if (strncmp(p, "${filename}", 11) == 0)
                {
                    strm << "${5}";
                    p += 11;
                }
                else if (strncmp(p, "${time}", 7) == 0)
                {
                    strm << "${6}";
                    p += 7;
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
            if (match_delim_ != '\0') {
                while (*src && *src != match_delim_ && src < matchingline.buf+matchingline.len)
                {
                    write(fd, src, 1);
                    ++src;
                }
            }
            if (match_additional_) {
                int n = match_additional_;
                while (*src && n && src < matchingline.buf+matchingline.len)
                {
                    write(fd, src, 1);
                    ++src;
                    --n;
                }
            }
            p += 4;
            return;
        }
        if (strncmp(p, "${5}", 4) == 0)     // ${filename}
        {
            std::string fname = theApp->filename();
            write (fd, fname.c_str(), fname.size());
            p += 4;
            return;
        }
        if (strncmp(p, "${6}", 4) == 0)     // ${time}
        {
            uint64_t t = TTLOG2micros(matchingline.buf, matchingline.len);
            char buf[32];
            int r = sprintf(buf, "%ld", t);
            if (r)
                write (fd, buf, r);
            p += 4;
            return;
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

    void LineSearch::on_exit(int fd)
    {
        lineSearch_->on_exit();
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

    MaxCount::MaxCount()
    :max_count_(0)
    ,current_count_(0)
    ,exit_(false)
    {
    }

    bool MaxCount::parse(const std::vector<std::string>& args)
    {
        for (auto& s : args)
        {
            if (s == "--exit")
                exit_ = true;
            else
            {
                try
                {
                    max_count_ = std::strtol(s.c_str(), nullptr, 10);
                }
                catch (std::exception& e)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void MaxCount::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        if (max_count_ == 0)
            return;     // disabled
        if (current_count_ == max_count_)
        {
            pattern_actions_->disable(true);
            if (exit_)
                theApp->exit();
        }
        ++current_count_;
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

    Count::Count()
    :count_(0)
    ,use_count_format_(false)
    {
    }

    bool Count::parse(const std::vector<std::string>& args)
    {
        for (auto& s : args)
        {
            const char *p = s.c_str();
            const char *end = p + s.size();
            while (*p)
            {
                if (*p == '\\')
                {
                    ++p;
                    if (*p == 'n')
                    {
                        format_.push_back('\n');
                    }
                    else if (*p == 't')
                    {
                        format_.push_back('\t');
                    }
                    else
                    {
                        format_.push_back(*p);
                    }
                    ++p;
                }
                else if ((end-p)>8 && memcmp(p, "${count}", 8) == 0)
                {
                    format_ += "%d";
                    if (use_count_format_)
                        return false;   // too many %d
                    use_count_format_ = true;
                    p += 8;
                }
                else
                {
                    format_.push_back(*p);
                    ++p;
                }
            }
        }
        return true;
    }

    void Count::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        ++count_;
    }

    void Count::on_exit(int fd)
    {
        if (use_count_format_)
            dprintf(fd, format_.c_str(), count_);
        else
            dprintf(fd, format_.c_str());
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
    ,stdout_(false)
    ,stderr_(false)
    {}

    bool File::parse(const std::vector<std::string>& args)
    {
        for (auto& s : args)
        {
            if (s == "--stdout")
                stdout_ = true;
            else if (s == "stderr")
                stderr_ = true;
            else if (s == "--append")
                append_ = true;
            else
                name_ = s;
        }

        if (stdout_ || stderr_)
            name_.clear();

        if (!name_.empty())
        {
            if (theApp->file(name_.c_str(), append_) < 0)
            {
                return false;
            }
        }
        return true;
    }

    void File::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        if (stdout_)
            pattern_actions_->fd_ = 0;
        else if (stderr_)
            pattern_actions_->fd_ = 1;
        else
            pattern_actions_->fd_ = theApp->file(name_.c_str(), append_);
    }

    /*********************************************************************/

    Interval::Interval()
    :lasttime_(0)
    ,use_time_format_(false)
    {
    }

    bool Interval::parse(const std::vector<std::string>& args)
    {
        for (auto& s : args)
        {
            const char *p = s.c_str();
            const char *end = p + s.size();
            while (*p)
            {
                if (*p == '\\')
                {
                    ++p;
                    if (*p == 'n')
                    {
                        format_.push_back('\n');
                    }
                    else if (*p == 't')
                    {
                        format_.push_back('\t');
                    }
                    else
                    {
                        format_.push_back(*p);
                    }
                    ++p;
                }
                else if ((end-p) >= 7 && memcmp(p, "${time}", 7) == 0)
                {
                    format_ += "%s";
                    p += 7;
                    if (use_time_format_)
                        return false;   // too many %d
                    use_time_format_ = true;
                }
                else
                {
                    format_.push_back(*p);
                    ++p;
                }
            }
        }
        return true;
    }

    void Interval::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        uint64_t ts = TTLOG2micros(matchingline.buf, matchingline.len);
        if (lasttime_ == 0)
        {
            lasttime_ = ts;
            return;
        }
        int64_t micros = (int64_t)ts - (int64_t)lasttime_;
        lasttime_ = ts;
        if (micros < 0)
            micros = -micros;
        std::string s;
        Micros2String(micros, s);
        if (use_time_format_)
            dprintf(fd, format_.c_str(), s.c_str());
        else
            dprintf(fd, format_.c_str());
    }

    void Interval::on_exit(int fd)
    {
    }

    /*********************************************************************/

    Action *ActionFactory(const std::string& name)
    {
        if (name == "print")
            return new Print();
        else if (name == "exit")
            return new Exit();
        else if (name == "file")
            return new File();
        else if (name == "max-count")
            return new MaxCount();
        else if (name == "count")
            return new Count();
        else if (name == "interval")
            return new Interval();
        //else if (name == "after")
            //return new After();
        //else if (name == "before")
            //return new Before();
        else
            return nullptr;
    }
}

