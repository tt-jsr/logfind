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
        pCtx_.reset(new AhoFileContext());
        pNamedPatternCtx_.reset(new AhoLineContext());
        exit_flag = false;
    }

    void Application::on_start()
    {
        int id = pNamedPatternCtx_->getPatternId("!program-start!");
        if (id >= 0)
        {
            struct aho_match_t m;
            m.id = id;
            m.line_match_pos = 0; 
            m.file_match_pos = 0;
            m.lineno = 0;
            m.line_position_in_file = 0;
            m.len = 15;
            pNamedPatternCtx_->on_match(&m);
        }
    }

    void Application::on_exit()
    {
        int id = pNamedPatternCtx_->getPatternId("!program-end!");
        if (id >= 0)
        {
            struct aho_match_t m;
            m.id = id;
            m.line_match_pos = 0; 
            m.file_match_pos = 0;
            m.lineno = 0;
            m.line_position_in_file = 0;
            m.len = 13;
            pNamedPatternCtx_->on_match(&m);
        }
    }

    void Application::on_file_start(const char *fname)
    {
        int id = pNamedPatternCtx_->getPatternId("!file-start!");
        if (id >= 0)
        {
            struct aho_match_t m;
            m.id = id;
            m.line_match_pos = 0; 
            m.file_match_pos = 0;
            m.lineno = 0;
            m.line_position_in_file = 0;
            m.len = 15;
            pNamedPatternCtx_->on_match(&m);
        }
    }

    void Application::on_file_end(const char *fname)
    {
        int id = pNamedPatternCtx_->getPatternId("!file-end!");
        if (id >= 0)
        {
            struct aho_match_t m;
            m.id = id;
            m.line_match_pos = 0; 
            m.file_match_pos = 0;
            m.lineno = 0;
            m.line_position_in_file = 0;
            m.len = 13;
            pNamedPatternCtx_->on_match(&m);
        }
    }
    void Application::exit()
    {
        exit_flag = true;
    }

    bool Application::is_exit()
    {
        return exit_flag;
    }

    std::string Application::filename()
    {
        return pCtx_->filename();
    }

    AhoFileContextPtr Application::search()
    {
        return pCtx_;
    }

    int Application::file(const char *name, bool append)
    {
        auto it = files_.find(name);
        if (it != files_.end())
            return it->second;

        int flags = O_WRONLY | O_CREAT;
        if (append)
            flags |= O_APPEND; 
        else
            flags |= O_TRUNC;
        int fd = open(name, flags, 0644);
        files_.emplace(name, fd);
        return fd;
    }

    AhoLineContextPtr Application::GetNamedContext(const std::string& name)
    {
        return pNamedPatternCtx_;
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
            break;
        case LINEBUF_INVALID:
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

