#include <cstdint>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "buffer.h"
#include "application.h"
#include "linebuf.h"
#include "lru_cache.h"
#include "aho_context.h"
#include "pattern_actions.h"
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
        PatternActionsPtr pa = pNamedPatternCtx_->getPatternActions("!program-start!");
        if (pa)
        {
            Match m;
            pa->on_match(m);
        }
    }

    void Application::on_exit()
    {
        PatternActionsPtr pa = pNamedPatternCtx_->getPatternActions("!program-end!");
        if (pa)
        {
            Match m;
            pa->on_match(m);
        }
    }

    void Application::on_file_start(const char *fname)
    {
        PatternActionsPtr pa = pNamedPatternCtx_->getPatternActions("!file-start!");
        if (pa)
        {
            Match m;
            pa->on_match(m);
        }
    }

    void Application::on_file_end(const char *fname)
    {
        PatternActionsPtr pa = pNamedPatternCtx_->getPatternActions("!file-end!");
        if (pa)
        {
            Match m;
            pa->on_match(m);
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
            assert(false);  // not implementd yet
            break;
        case LINEBUF_NONE:
            break;
        case LINEBUF_INVALID:
            break;
        }
        lb.buf = nullptr;
        lb.bufsize = 0;
        lb.flags = LINEBUF_INVALID;
    }

    bool Application::alloc(linebuf& lb)
    {
        if (lb.buf)
            free(lb);
        lb.buf = new char[1024*1024];
        lb.bufsize = 1024*1024;
        lb.len = 0;
        lb.flags = LINEBUF_FREE;
    }

    Data *Application::getData(const std::string& name)
    {
        auto it = data_.find(name);
        if (it == data_.end())
            return nullptr;
        return it->second;
    }

    void Application::setData(const std::string& name, Data * d)
    {
        auto it = data_.find(name);
        if (it == data_.end())
        {
            data_[name] = d;
            return;
        }
        if (it->second != d)
            delete it->second;
        data_[name] = d;
    }

    extern Application *theApp;
}

