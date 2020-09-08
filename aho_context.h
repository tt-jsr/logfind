#ifndef AHO_CONTEXT_H_
#define AHO_CONTEXT_H_

#include <cstdint>
#include <vector>
#include <string.h>
#include "file.h"

extern "C"
{
#include "ahocorasick.h"
}

extern "C" void callback_match(void *arg, struct aho_match_t* m);
extern "C" char aho_getchar(void *arg);

namespace logfind
{
    class AhoContext
    {
    public:
        AhoContext()
        {
            aho_init(&aho_);
            aho_register_match_callback(&aho_, callback_match, this);
        }
        ~AhoContext()
        {
            aho_destroy(&aho_);
        }

        void add_match_text(const char *p, uint32_t len)
        {
            int id = aho_add_match_text(&aho_, p, len);
            ids_.push_back(id);
        }

        void add_match_text(const char *p)
        {
            add_match_text(p, strlen(p));
        }

        virtual char get() = 0;
        virtual void on_match(struct aho_match_t *) = 0;
    protected:

        struct ahocorasick aho_;
        std::vector<int> ids_;
    };

    class AhoFileContext : public AhoContext
    {
    public:
        bool open(const char *fname)
        {
            return file.open(fname);
        }

        void start()
        {
            aho_create_trie(&aho_);
            aho_findtext(&aho_, aho_getchar, this);
        }

    protected:
        ReadFile file;
    private:
        char get() override
        {
            return file.get();
        }
    };
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

#endif

