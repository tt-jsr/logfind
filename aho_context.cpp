#include "aho_context.h"
#include <assert.h>

extern "C" void callback_match(void *arg, struct aho_match_t* m);
extern "C" char aho_getchar(void *arg);

namespace logfind
{
    AhoContext::AhoContext()
    {
        aho_init(&aho_);
        aho_register_match_callback(&aho_, callback_match, this);
    }
    AhoContext::~AhoContext()
    {
        aho_destroy(&aho_);
    }

    int AhoContext::add_match_text(const char *p, uint32_t len, PatternActions& actions)
    {
        int id = aho_add_match_text(&aho_, p, len);
        match_actions.emplace(id, actions);
        return id;
    }

    int AhoContext::add_match_text(const char *p, PatternActions& actions)
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
        it->second.on_match(this, m);
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
        return file.get();
    }

    void AhoFileContext::readLine(uint64_t lineno, char *buf, uint32_t len)
    {
        file.readLine(lineno, buf, len);
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
        if (pos == line.size())
            return 0;
        return line[pos++];
    }

    void AhoLineContext::readLine(uint64_t lineno, char *buf, uint32_t len)
    {
        strncpy(buf, line.c_str(), len);
        buf[len-1] = '\0';
    }

    /*****************************************************************/
    void PatternActions::on_match(AhoContext *ctx, struct aho_match_t* m)
    {
        char buf[4096];
        ctx->readLine(m->lineno, buf, sizeof(buf));
        printf("%s\n", buf);
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

