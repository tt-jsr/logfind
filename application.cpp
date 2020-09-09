#include <cstdint>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "application.h"
#include "linebuf.h"
#include "lru_cache.h"
#include "aho_context.h"
#include <assert.h>

namespace logfind
{
    Application *theApp = nullptr;

    Application::Application()
    {
        theApp = this;
        pCtx.reset(new AhoFileContext());
        exit_flag = false;
    }

    void Application::exit()
    {
        exit_flag = true;
    }

    bool Application::is_exit()
    {
        return exit_flag;
    }

    int Application::file(const char *name, bool append)
    {
        auto it = files_.find(name);
        if (it != files_.end())
            return it->second;

        int flags = O_WRONLY | O_CREAT;
        if (append)
            flags |= O_APPEND; 
        int fd = open(name, flags, 0644);
        files_.emplace(name, fd);
        return fd;
    }

    void Application::NamedPatternAction(const char *name, PatternActionsPtr pa)
    {
        named_patterns_.emplace(name, pa);
    }

    PatternActionsPtr Application::GetNamedPattern(const char *name)
    {
        auto it = named_patterns_.find(name);
        if (it != named_patterns_.end())
            return it->second;
        return PatternActionsPtr();
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

