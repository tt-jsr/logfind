#include <stdio.h>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"


void test()
{
    logfind::Application app;

    auto fileSearch = app.search();
    auto pa = fileSearch->add_match_text("S T A R T");
    pa->print();
    pa = fileSearch->add_match_text("Orderserver is ready");
    pa->print();
    pa = fileSearch->add_match_text("loadbalancer invoked");
    pa->print();
    pa = fileSearch->add_match_text("build");
    pa->print();
    fileSearch->build_trie();

    if (fileSearch->find("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
    }
}

void test2()
{
    logfind::Application app;
    auto fileSearch = app.search();
    auto pa = fileSearch->add_match_text("bb77a423-f475-434b-a336-3bd09a32c464");
    fileSearch->build_trie();

    auto lineSearch = pa->search();
    pa = lineSearch->add_match_text("EXEC_TYPE_TRADE");
    pa->print();
    pa = lineSearch->add_match_text("EXEC_TYPE_NEW");
    pa->print();
    lineSearch->build_trie();

    if (fileSearch->find("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
    }
}

int main(int /*argc*/, char ** /*argv*/)
{
    test2();
    return 0;
}

