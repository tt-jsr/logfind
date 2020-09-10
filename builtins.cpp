#include "lru_cache.h"
#include "aho_context.h"
#include "file.h"
#include "linebuf.h"
#include "application.h"
#include "builtins.h"
#include "pattern_actions.h"

namespace logfind
{
    void After::parse(const std::vector<std::string>& args)
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

    void Before::parse(const std::vector<std::string>& args)
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

    void Print::parse(const std::vector<std::string>& args)
    {
    }

    void Print::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        dprintf(fd, "%s\n", matchingline.buf);
    }

    /*********************************************************************/

    LineSearch::LineSearch()
    {
        lineSearch_ = MakeAhoLineContext();
    }

    void LineSearch::parse(const std::vector<std::string>& args)
    {
    }

    void LineSearch::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        lineSearch_->find(matchingline.buf, matchingline.len);
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

    void Exit::parse(const std::vector<std::string>& args)
    {
    }

    void Exit::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        theApp->exit();
    }

    /*********************************************************************/

    void NamedPatternActions::parse(const std::vector<std::string>& args)
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

    void File::parse(const std::vector<std::string>& args)
    {
    }

    void File::on_command(int fd, uint32_t lineno, linebuf& matchingline)
    {
        pattern_actions_->fd_ = theApp->file(name_.c_str(), append_);
    }

    /*********************************************************************/

    Builtin *BuiltinFactory(const std::string& name)
    {
        if (name == "after")
            return new After();
        else if (name == "before")
            return new Before();
        else if (name == "print")
            return new Print();
        else if (name == "exit")
            return new Exit();
        else if (name == "file")
            return new File();
        else
            return nullptr;
    }
}

