#include <cstdint>
#include <assert.h>
#include "application.h"
#include "linebuf.h"
#include "lru_cache.h"
#include "aho_context.h"

namespace logfind
{
    Application *theApp = nullptr;

    Application::Application()
    {
        theApp = this;
        pCtx.reset(new AhoFileContext());
    }

    void Application::free(linebuf& lb)
    {
        switch (lb.flags)
        {
        case LINEBUF_FREE:
            delete [] lb.buf;
            break;
        case LINEBUF_POOL:
            assert(false);
            break;
        case LINEBUF_NONE:
            assert(false);
            break;
        }
    }

    bool Application::alloc(linebuf& lb)
    {
        lb.buf = new char[1024*1024];
        lb.bufsize = 1024*1024;
        lb.len = 0;
        lb.flags = LINEBUF_FREE;
    }

    extern Application *theApp;
}

