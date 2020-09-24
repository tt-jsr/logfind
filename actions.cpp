#include <sstream>
#include <unistd.h>
#include <limits>
#include <assert.h>
#include <iostream>
#include <iterator>
#include <regex>
#include "buffer.h"
#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "actions.h"
#include "pattern_actions.h"
#include "utilities.h"

extern int on_match(int strnum, int textpos, void *v);
extern void on_line(int textpos, void *v);

namespace logfind
{
    /*********************************************************************/

    bool After::parse(const std::vector<std::string>& args)
    {
    }

    void After::on_command(int fd, Match& match)
    {
    }

    /*********************************************************************/

    bool Before::parse(const std::vector<std::string>& args)
    {
    }

    void Before::on_command(int fd, Match& match)
    {
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
                else if (strncmp(p, "${rotate-time}", 14) == 0)
                {
                    strm << "${7}";
                    p += 14;
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

    void Print::substitute(const char *&p, int fd, Match& match)
    {
        if (strncmp(p, "${1}", 4) == 0)  //${lineno}
        {
            char buf[32];
            sprintf (buf, "%d", match.lineno);
            write(fd, buf, strlen(buf));
            p += 4;
            return;
        }
        else if (strncmp(p, "${2}", 4) == 0)     // ${offset}
        {
            char buf[32];
            sprintf (buf, "%d", match.match_offset_in_file);

            write(fd, buf, strlen(buf));
            p += 4;
            return;
        }
        else if (strncmp(p, "${3}", 4) == 0)     // ${line}
        {
            int start(line_start_), end(line_end_);
            if (start > (int)match.matched_line.len)
            {
                start = (int)match.matched_line.len;
            }
            if (start < 0)
            {
                start = (int)match.matched_line.len + start;
                if (start < 0)
                    start = 0;
            }
            if (end > (int)match.matched_line.len)
                end = (int)match.matched_line.len;
            if (end < 0)
            {
                end = (int)match.matched_line.len + end;
                if (end < 0)
                    end = 0;
            }
            if (start > end)
                start = end;

            int len = end-start;
            write(fd, match.matched_line.buf+start, len);
            p += 4;
            return;
        }

        else if (strncmp(p, "${4}", 4) == 0)     // ${match}
        {
            const char *src = match.matched_line.buf + match.match_offset_in_line;
            write(fd, src, match.matched_text.size());
            src += match.matched_text.size();
            if (match_delim_ != '\0') {
                while (*src && *src != match_delim_ && src < match.matched_line.buf+match.matched_line.len)
                {
                    write(fd, src, 1);
                    ++src;
                }
            }
            if (match_additional_) {
                int n = match_additional_;
                while (*src && n && src < match.matched_line.buf+match.matched_line.len)
                {
                    write(fd, src, 1);
                    ++src;
                    --n;
                }
            }
            p += 4;
            return;
        }
        else if (strncmp(p, "${5}", 4) == 0)     // ${filename}
        {
            std::string fname = theApp->filename();
            write (fd, fname.c_str(), fname.size());
            p += 4;
            return;
        }
        else if (strncmp(p, "${6}", 4) == 0)     // ${time}
        {
            uint64_t t = TTLOG2micros(match.matched_line.buf, match.matched_line.len);
            char buf[32];
            int r = sprintf(buf, "%ld", t);
            if (r)
                write (fd, buf, r);
            p += 4;
            return;
        }
        else if (strncmp(p, "${7}", 4) == 0)     // ${rotate-time}
        {
            uint64_t ts = TTLOGRotateTime(theApp->filename());
            if (ts)
            {
                time_t t = ts/1000000;
                struct tm *tm = gmtime(&t);
                char buf[64];
                int r = sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                if (r)
                    write (fd, buf, r);
            }
            else
            {
                write (fd, "n/a", 3);
            }
            p += 4;
            return;
        }
        else
        {
            const char *save = p;
            p += 2;
            std::string name;
            while (*p && *p != '}')
            {
                name.push_back(*p);
                ++p;
            }
            if (*p)
                ++p;
            Data *d = theApp->getData(name);
            if (d)
            {
                std::string s = d->tostr();
                write(fd, s.c_str(), s.size());
            }
            else
            {
                write(fd, save, p-save);
            }
        }
    }

    void Print::on_command(int fd, Match& match)
    {
        if (format_.empty())
        {
            dprintf(fd, "%s\n", match.matched_line.buf);
            return;
        }
        const char * p = format_.c_str();
        while (*p)
        {
            if (*p == '$' && *(p+1) == '{')
            {
                substitute(p, fd, match);
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
    :acism_(nullptr)
    ,pattv_(nullptr)
    ,npatts_(0)
    {
        lineSearch_ = MakeAhoLineContext();
    }

    LineSearch::~LineSearch()
    {
        // prob should do some cleanup
    }

    bool LineSearch::parse(const std::vector<std::string>& args)
    {
    }

    void LineSearch::on_command(int fd, Match& match)
    {
        lineSearch_->matched_line_.assign(match.matched_line.buf, match.matched_line.len);
        lineSearch_->matched_lineno_ = match.lineno;
        if (acism_ == nullptr)
        {
            pattv_ = new MEMREF[lineSearch_->patterns_.size()];
            npatts_ = lineSearch_->patterns_.size();
            for (size_t i = 0; i < lineSearch_->patterns_.size(); i++)
            {
                (pattv_[i]).ptr = lineSearch_->patterns_[i].c_str();  // I am not copying here!
                (pattv_[i]).len = lineSearch_->patterns_[i].size();
            }

            // Create the context
            acism_ = acism_create(pattv_, lineSearch_->patterns_.size());
        }
        int state(0);
        MEMREF block;
        block.ptr = match.matched_line.buf;
        block.len = match.matched_line.len;

        (void)acism_more(acism_, block, (ACISM_ACTION*)::on_match, (ACISM_LINE*)::on_line, lineSearch_.get(), &state);
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

    void MaxCount::on_command(int fd, Match& match)
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

    void Exit::on_command(int fd, Match& match)
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

    void Count::on_command(int fd, Match& match)
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
            else if (s == "--stderr")
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

    void File::on_command(int fd, Match& match)
    {
        if (stdout_)
            pattern_actions_->fd_ = 1;
        else if (stderr_)
            pattern_actions_->fd_ = 2;
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

    void Interval::on_command(int fd, Match& match)
    {
        uint64_t ts = TTLOG2micros(match.matched_line.buf, match.matched_line.len);
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

    Regex::Regex()
    {

    }
    bool Regex::parse(const std::vector<std::string>& args) 
    {
        if (args.size() != 2)
            return false;
        regex_ = std::regex(args[0], std::regex_constants::egrep);
        varname_ = args[1];
        return true;
    }

    void Regex::on_command(int fd, Match& m)
    {
        std::smatch match;
        std::string line(m.matched_line.buf, m.matched_line.len);
        if (std::regex_search(line, match, regex_))
        {
            if (match.size() == 1)
            {
                StringData *pd = new StringData();
                pd->str = match[0];
                theApp->setData(varname_, pd);
            }
            if (match.size() >= 2)
            {
                StringData *pd = new StringData();
                pd->str = match[1];
                theApp->setData(varname_, pd);
            }
        }
    }

    void Regex::on_exit(int fd)
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
        else if (name == "regex")
            return new Regex();
        //else if (name == "after")
            //return new After();
        //else if (name == "before")
            //return new Before();
        else
            return nullptr;
    }
}

