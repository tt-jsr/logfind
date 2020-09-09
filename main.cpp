#include <stdio.h>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"


void test()
{
    logfind::Application app;
    auto pa = logfind::MakePatternActions();
    pa->print();

    app.pCtx->add_match_text("S T A R T", pa);
    app.pCtx->add_match_text("Orderserver is ready", pa);
    app.pCtx->add_match_text("loadbalancer invoked", pa);
    app.pCtx->add_match_text("build", pa);
    app.pCtx->build_trie();

    if (app.pCtx->find("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
    }
}

void test2()
{
    logfind::Application app;
    app.pCtx->add_match_text("bb77a423-f475-434b-a336-3bd09a32c464", logfind::MakePatternActions());
    app.pCtx->build_trie();

    if (app.pCtx->find("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
    }
}

int main(int /*argc*/, char ** /*argv*/)
{
    test();
    return 0;
}

