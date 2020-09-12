#include <stdio.h>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"
#include "pattern_actions.h"
#include "builtins.h"
#include "parse.h"


void test()
{
    logfind::Application app;

    auto fileSearch = app.search();
    auto pa = fileSearch->add_match_text("S T A R T");
    pa->add_command(new logfind::Print());
    pa = fileSearch->add_match_text("Orderserver is ready");
    pa->add_command(new logfind::Print());
    pa = fileSearch->add_match_text("loadbalancer invoked");
    pa->add_command(new logfind::Print());
    pa = fileSearch->add_match_text("build");
    pa->add_command(new logfind::Print());
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
    auto filePa = fileSearch->add_match_text("bb77a423-f475-434b-a336-3bd09a32c464");
    fileSearch->build_trie();

    logfind::LineSearch *searchCmd = new logfind::LineSearch();
    filePa->add_command(searchCmd);
    auto pa = searchCmd->add_match_text("EXEC_TYPE_TRADE");
    pa->add_command(new logfind::Print());
    pa = searchCmd->add_match_text("EXEC_TYPE_NEW");
    pa->add_command(new logfind::Print());
    searchCmd->build_trie();

    if (fileSearch->find("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
    }
}

int file_test1()
{
    logfind::Application app;
    logfind::Parse parse;
    if (parse.parse("test.lf") == false)
    {
        printf ("Failed to open/parse test.lf\n");
        return 1;
    }

    logfind::AhoFileContextPtr ptr = app.search();
    if (ptr->find("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
        return 1;
    }
    return 0;
}

int file_test2()
{
    logfind::Application app;
    logfind::Parse parse;
    if (parse.parse("test2.lf") == false)
    {
        printf ("Failed to open/parse test2.lf\n");
        return 1;
    }

    logfind::AhoFileContextPtr ptr = app.search();
    if (ptr->find("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
        return 1;
    }
    app.on_exit();
    return 0;
}

int main(int /*argc*/, char ** /*argv*/)
{
    return file_test2();
}

