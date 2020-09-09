#include <assert.h>
#include <cstdint>
#include "lru_cache.h"
#include "linebuf.h"
#include "aho_context.h"
#include "application.h"

extern "C" void callback_match(void *arg, struct aho_match_t* m);
extern "C" char aho_getchar(void *arg);

namespace logfind
{
    PatternActionsPtr MakePatternActions()
    {
        return std::make_shared<PatternActions>();
    }

    AhoContext::AhoContext()
    {
        aho_init(&aho_);
        aho_register_match_callback(&aho_, callback_match, this);
    }
    AhoContext::~AhoContext()
    {
        aho_destroy(&aho_);
    }

    int AhoContext::add_match_text(const char *p, uint32_t len, PatternActionsPtr actions)
    {
        int id = aho_add_match_text(&aho_, p, len);
        match_actions.emplace(id, actions);
        return id;
    }

    int AhoContext::add_match_text(const char *p, PatternActionsPtr actions)
    {
        return add_match_text(p, strlen(p), actions);
    }

    void AhoContext::build_trie()
    {
        aho_create_trie(&aho_);
    }

    void AhoContext::on_match(struct aho_match_t* m)
    {
        auto it = match_actions.find(m->id);
        assert(it != match_actions.end());
        it->second->on_match(this, m);
    }

    /**************************************************************/
    bool AhoFileContext::find(const char *fname)
    {
        bool r = file.open(fname);
        if (r == false)
            return false;
        aho_findtext(&aho_, aho_getchar, this);
        return true;
    }

    char AhoFileContext::get() 
    {
        if (theApp->is_exit())
            return 0;
        return file.get();
    }

    bool AhoFileContext::readLine(uint64_t lineno, linebuf& lb)
    {
        return file.readLine(lineno, lb);
    }

    /*********************************************************************/
    AhoLineContext::AhoLineContext()
    :pos(0)
    {
    }

    bool AhoLineContext::find(const char *p, uint32_t len)
    {
        pos = 0;
        line.assign(p, len);
        aho_findtext(&aho_, aho_getchar, this);
    }

    char AhoLineContext::get()
    {
        if (theApp->is_exit())
            return 0;
        if (pos == line.size())
            return 0;
        return line[pos++];
    }

    bool AhoLineContext::readLine(uint64_t lineno, linebuf& lb)
    {
        lb.buf = (char *)line.c_str();
        lb.len = line.size();
        lb.bufsize = line.capacity();
        lb.flags = LINEBUF_NONE;
    }

    /*****************************************************************/
    PatternActions::PatternActions()
    :before_(0)
    ,after_(0)
    ,fd_(1)
    {
    }
    void PatternActions::on_match(AhoContext *ctx, struct aho_match_t* m)
    {
        for (std::string& cmd : commands_)
        {
            if (cmd == "after")
            {
                printlines(ctx, m, m->lineno, m->lineno+after_);
            }
            else if (cmd == "before")
            {
                uint32_t start = m->lineno-before_;
                if (before_ > m->lineno)
                    start = 0;
                printlines(ctx, m, start, m->lineno);
            }
            else if (cmd == "search")
            {
                linebuf lb;
                ctx->readLine(m->lineno, lb);
                pCtx_->find(lb.buf, lb.len);
                theApp->free(lb);
            }
            else if (cmd == "print")
            {
                linebuf lb;
                ctx->readLine(m->lineno, lb);
                dprintf(fd_, "%s\n", lb.buf);
                theApp->free(lb);
            }
            else if (cmd == "exit")
            {
                theApp->exit();
            }
            else
            {
                // must be a named pattern actions
                PatternActionsPtr pa = theApp->GetNamedPattern(cmd.c_str());
                if (pa)
                {
                    pa->on_match(ctx, m);
                }
            }
        }
    }

    void PatternActions::printlines(AhoContext *ctx, struct aho_match_t* m, int32_t start, uint32_t end)
    {
        linebuf lb;
        for (uint32_t l = start; l < end; ++l)
        {
            ctx->readLine(l, lb);
            dprintf(fd_, "%s\n", lb.buf);
            theApp->free(lb);
        }
    }

    void PatternActions::before(uint8_t n)
    {
        if (before_)
            return;
        commands_.push_back("before");
        before_ = n;
    }

    void PatternActions::after(uint8_t n)
    {
        if (after_)
            return;
        commands_.push_back("after");
        after_ = n;
    }

    void PatternActions::print()
    {
        commands_.push_back("print");
    }

    void PatternActions::file(const char *name, bool append)
    {
        fd_ = theApp->file(name, append);
    }
    
    void PatternActions::search(std::shared_ptr<AhoLineContext> ctx)
    {
        if (pCtx_)
            return;
        commands_.push_back("search");
        pCtx_ = ctx;
    }

    void PatternActions::named_actions(const char *name)
    {
        commands_.push_back(name);
    }
}


extern "C" char aho_getchar(void *arg)
{
    logfind::AhoContext *pCtx = (logfind::AhoContext *)arg;
    return pCtx->get();
}

extern "C" void callback_match(void *arg, struct aho_match_t* m)
{
    logfind::AhoContext *pCtx = (logfind::AhoContext *)arg;
    pCtx->on_match(m);
}

