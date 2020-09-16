#include <assert.h>
#include <cstdint>
#include "lru_cache.h"
#include "linebuf.h"
#include "aho_context.h"
#include "application.h"
#include "pattern_actions.h"

void callback_match(void *arg, struct aho_match_t* m);
char aho_getchar(void *arg);

namespace logfind
{
    AhoLineContextPtr MakeAhoLineContext()
    {
        return std::make_shared<AhoLineContext>();
    }

    AhoFileContextPtr MakeAhoFileContext()
    {
        return std::make_shared<AhoFileContext>();
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

    int AhoContext::getPatternId(const char *p)
    {
        auto it = match_str_actions.find(p);
        if (it != match_str_actions.end())
            return it->second;
        return -1;
    }

    PatternActionsPtr AhoContext::add_match_text(const char *p, uint32_t len)
    {
        auto pa = logfind::MakePatternActions();
        int id = aho_add_match_text(&aho_, p, len);
        match_actions.emplace(id, pa);
        match_str_actions.emplace(std::string(p, len), id);
        return pa;
    }

    PatternActionsPtr AhoContext::add_match_text(const char *p)
    {
        return add_match_text(p, strlen(p));
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

    void AhoContext::on_exit()
    {
        for (auto& pr : match_actions)
        {
            pr.second->on_exit(this);
        }
    }

    /**************************************************************/
    bool AhoFileContext::find(const char *fname)
    {
        bool r = file.open(fname);
        if (r == false)
        {
            return false;
        }
        theApp->on_file_start(fname);
        aho_findtext(&aho_, 0, 0, aho_getchar, this);
        theApp->on_file_end(fname);
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

    bool AhoLineContext::find(const char *p, uint32_t len, uint32_t lineno, uint64_t lineoff)
    {
        pos = 0;
        line.assign(p, len);
        aho_findtext(&aho_, lineno, lineoff, aho_getchar, this);
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
}

char aho_getchar(void *arg)
{
    logfind::AhoContext *pCtx = (logfind::AhoContext *)arg;
    return pCtx->get();
}

void callback_match(void *arg, struct aho_match_t* m)
{
    logfind::AhoContext *pCtx = (logfind::AhoContext *)arg;
    pCtx->on_match(m);
}

