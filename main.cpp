#include <stdio.h>
#include <iostream>
#include <map>
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"
#include "pattern_actions.h"
#include "actions.h"
#include "parse.h"
#include "utilities.h"

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
    std::string fileprefix;
    std::string timestamp;

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
        else if (strcmp (argv[a], "--prefix") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--prefix requires filename prefix" << std::endl;
                return 1;
            }
            fileprefix = argv[a];
        }
        else if (strcmp (argv[a], "--time") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--time requires TTLOG timestamp" << std::endl;
                return 1;
            }
            timestamp = argv[a];
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

    if (!timestamp.empty() && fileprefix.empty())
    {
        std::cerr << "--time requires --prefix" << std::endl;
        return 1;
    }

    if (nodash.size() == 1)
    {
        infile = nodash[0];
    }
    if (nodash.size() > 1)
    {
        infile = nodash.back();
        nodash.pop_back();
    }

    if (infile.empty())
        infile = "-";

    // Instantiate the app
    logfind::Application app;

    // Parse the script, if any
    if (!script.empty())
    {
        logfind::Parse parse;
        if (parse.parse(script.c_str()) == false)
        {
            std::cerr << "Failed to open/parse " << script << std::endl;
            return 1;
        }
    }

    // Add any additional patterns that might have been on the command line
    AddPatterns(app, nodash);

    std::map<uint64_t, logfind::FileInfo> files;
    if (!fileprefix.empty())
    {
        logfind::GetFileInfos(fileprefix, files);
        for (auto& pr : files)
        {
            std::cerr << pr.second.filepath << ":" << pr.second.sTimestamp << std::endl;
        }
    }

    // let'r rip...
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

