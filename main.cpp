#include <stdio.h>
#include "aho_context.h"


void test()
{
    logfind::AhoFileContext ctx;
    logfind::PatternActions pa;
    ctx.add_match_text("S T A R T", pa);
    ctx.add_match_text("Orderserver is ready", pa);
    ctx.add_match_text("loadbalancer invoked", pa);
    ctx.add_match_text("build", pa);
    ctx.build_trie();

    if (ctx.find("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
    }
}

void test2()
{
    logfind::AhoFileContext ctx;
    logfind::PatternActions pa;
    ctx.add_match_text("bb77a423-f475-434b-a336-3bd09a32c464", pa);
    ctx.build_trie();

    if (ctx.find("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
    }
}

int main(int /*argc*/, char ** /*argv*/)
{
    test();
    return 0;
}

