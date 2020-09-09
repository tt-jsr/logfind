#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <memory>

namespace logfind
{
    class AhoFileContext;
    struct linebuf;

    class Application
    {
    public:
        Application();

        void free(linebuf& lb);
        bool alloc(linebuf& lb);

        std::unique_ptr<AhoFileContext> pCtx;
    };

    extern Application *theApp;
}

#endif
