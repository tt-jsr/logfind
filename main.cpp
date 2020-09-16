#include <stdio.h>
#include <iostream>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"
#include "pattern_actions.h"
#include "actions.h"
#include "parse.h"

static const char *version = ".9";

void usage()
{
    std::cout << "usage: logfind [--script file] [--infile file] [--pattern]" << std::endl;
    std::cout << "       logfind pattern pattern ... file" << std::endl;
    std::cout << "               file can be '-' for stdin" << std::endl;
    std::cout << "       logfind --version" << std::endl;
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
    std::string infile;
    std::vector<std::string> nodash;

    for (int a = 1; a < argc; ++a)
    {
        if (strcmp (argv[a], "--help") == 0 || strcmp(argv[a], "-h") == 0)
        {
            usage();
            return 0;
        }
        else if (strcmp (argv[a], "--script") == 0)
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
            nodash.push_back(std::string(argv[a]));
        }
        else if (strcmp (argv[a], "--version") == 0 || strcmp(argv[a], "-v") == 0)
        {
            std::cerr << version << std::endl;
            return 0;
        }
        else if (strcmp (argv[a], "-") == 0)
        {
            infile = "-";
        }
        else if (argv[a][0] == '-')
        {
            std::cerr << "Unknown argument: " << argv[a] << std::endl;
            usage();
            return 0;
        }
        else
        {
            nodash.push_back(std::string(argv[a]));
        }
    }

    logfind::Application app;
    if (!script.empty())
    {
        logfind::Parse parse;
        if (parse.parse(script.c_str()) == false)
        {
            std::cerr << "Failed to open/parse " << script << std::endl;
            return 1;
        }
    }

    if (nodash.size() == 1)
    {
        // we have a pattern and are going to read from stdin/infile
        AddPatterns(app, nodash);
    }
    if (nodash.size() > 1)
    {
        if (!infile.empty())
        {
            // We have specified the input file, so everthing is a pattern
            AddPatterns(app, nodash);
        }
        else
        {
            // We haven't specified the infile, so the last item is the file
            infile = nodash.back();
            nodash.pop_back();
            AddPatterns(app, nodash);
        }
    }

    if (infile.empty())
        infile = "-";
    app.on_start();
    logfind::AhoFileContextPtr ptr = app.search();
    if (ptr->find(infile.c_str()) == false)
    {
        std::cerr << "Failed to open " << infile << std::endl;
        return 1;
    }
    app.on_exit();
    return 0;
}

