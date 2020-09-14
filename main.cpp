#include <stdio.h>
#include <iostream>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"
#include "pattern_actions.h"
#include "actions.h"
#include "parse.h"


void test()
{
    logfind::Application app;

    auto fileSearch = app.search();
    auto pa = fileSearch->add_match_text("S T A R T");
    pa->add_action(new logfind::Print());
    pa = fileSearch->add_match_text("Orderserver is ready");
    pa->add_action(new logfind::Print());
    pa = fileSearch->add_match_text("loadbalancer invoked");
    pa->add_action(new logfind::Print());
    pa = fileSearch->add_match_text("build");
    pa->add_action(new logfind::Print());
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
    filePa->add_action(searchCmd);
    auto pa = searchCmd->add_match_text("EXEC_TYPE_TRADE");
    pa->add_action(new logfind::Print());
    pa = searchCmd->add_match_text("EXEC_TYPE_NEW");
    pa->add_action(new logfind::Print());
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

void usage()
{
    std::cout << "usage: logfind ..." << std::endl;
}

void AddPatterns(logfind::Application& app, std::vector<std::string>& patterns)
{
    auto fileSearch = app.search();
    for (auto& pat : patterns)
    {
        auto filePa = fileSearch->add_match_text(pat.c_str());
        filePa->add_action(new logfind::Print());
    }
    fileSearch->build_trie();
}

int main(int argc, char ** argv)
{
    std::string script;
    std::string outfile;
    std::string infile;
    std::vector<std::string> patterns;

    for (int a = 1; a < argc; ++a)
    {
        if (strcmp (argv[a], "--script") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--script requires filename" << std::endl;
                return 1;
            }
            script = argv[a];
        }
        else if (strcmp (argv[a], "--outfile") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--outfile requires filename" << std::endl;
                return 1;
            }
            outfile = argv[a];
        }
        else if (strcmp (argv[a], "--infile") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--infile requires filename" << std::endl;
                return 1;
            }
            infile = argv[a];
        }

        else if (strcmp (argv[a], "--pattern") == 0 || strcmp(argv[a], "-e") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--pattern requires an argument" << std::endl;
                return 1;
            }
            std::cout << "adding " << argv[a] << std::endl;
            patterns.push_back(std::string(argv[a]));
        }
        else
        {
            std::cout << "Unknown argument: " << argv[a] << std::endl;
            return 1;
        }
    }

    logfind::Application app;
    if (!script.empty())
    {
        logfind::Parse parse;
        if (parse.parse(script.c_str()) == false)
        {
            std::cout << "Failed to open/parse " << script << std::endl;
            return 1;
        }
    }

    AddPatterns(app, patterns);

    logfind::AhoFileContextPtr ptr = app.search();
    if (ptr->find(infile.c_str()) == false)
    {
        std::cout << "Failed to open " << infile << std::endl;
        return 1;
    }
    app.on_exit();
    return 0;

    return file_test2();
}

