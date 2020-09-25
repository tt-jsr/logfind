#include <stdio.h>
#include <iostream>
#include <map>
#include "buffer.h"
#include "lru_cache.h"
#include "application.h"
#include "aho_context.h"
#include "pattern_actions.h"
#include "actions.h"
#include "parse.h"
#include "utilities.h"
#include "list_cmd.h"
#include "cat_cmd.h"

static const char *version = ".1";

void usage()
{
    std::cerr << "usage: " << std::endl;
}

void AddPatterns(logfind::Application& app, std::vector<std::string>& patterns)
{
    auto fileSearch = app.search();
    for (auto& pat : patterns)
    {
        auto filePa = fileSearch->add_match_text(pat.c_str());
        filePa->add_action(new logfind::Print());
    }
}

int main(int argc, char ** argv)
{
    bool cat(false);
    bool list(false);
    bool search(false);
    bool dash_1(false);

    std::string script;
    std::vector<std::string> positional_args;
    std::string logname;
    std::string before;
    std::string after;
    std::string locate;
    std::string split;

    if (argc == 1)
    {
        usage();
        return 0;
    }
    if (strcmp (argv[1], "cat") == 0)
        cat = true;
    else if (strcmp (argv[1], "list") == 0)
        list = true;
    else if (strcmp (argv[1], "search") == 0)
        search = true;
    else if (strcmp (argv[1], "--help") == 0)
    {
        usage();
        return 0;
    }
    else
    {
        std::cerr << "Unknown command " <<argv[1] << std::endl;
        usage();
        return 1;
    }

    if (argc < 3)
    {
        usage();
        return 0;
    }
    logname = argv[2];

    for (int a = 3; a < argc; ++a)
    {
        if (strcmp (argv[a], "--help") == 0 || strcmp(argv[a], "-h") == 0)
        {
            usage();
            return 0;
        }
        else if (strcmp (argv[a], "--script") == 0 || strcmp(argv[a], "-s") == 0)
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
        else if (strcmp (argv[a], "--split") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--split requires filename" << std::endl;
                return 1;
            }
            split = argv[a];
        }
        else if (strcmp (argv[a], "--before") == 0 || strcmp(argv[a], "-b") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--before requires time spec" << std::endl;
                return 1;
            }
            before = argv[a];
        }
        else if (strcmp (argv[a], "--after") == 0 || strcmp(argv[a], "-a") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--after requires time spec" << std::endl;
                return 1;
            }
            after = argv[a];
        }
        else if (strcmp (argv[a], "--locate") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--locate requires time" << std::endl;
                return 1;
            }
            locate = argv[a];
        }
        else if (strcmp (argv[a], "-1") == 0)
        {
            dash_1 = true;
        }
        else if (strcmp (argv[a], "--version") == 0 || strcmp(argv[a], "-v") == 0)
        {
            std::cerr << version << std::endl;
            return 0;
        }
        else if (argv[a][0] == '-')
        {
            std::cerr << "Unknown argument: " << argv[a] << std::endl;
            usage();
            return 0;
        }
        else
        {
            positional_args.push_back(std::string(argv[a]));
        }
    }

    if (list)
    {
        if (cat)
        {
            std::cerr << "--cat and --list cannot be used together" << std::endl;
            return 1;
        }
        if (!script.empty())
        {
            std::cerr << "--script and --list cannot be used together" << std::endl;
            return 1;
        }
        if (logname.empty())
        {
            std::cerr << "--list requires --logname" << std::endl;
            return 1;
        }
        if (dash_1)
            std::cerr << "-1 will be ignored" << std::endl;
        if (logname == "-")
        {
            std::cerr << "stdin not allowed for list" << std::endl;
            return 1;
        }
        if (locate.empty() && positional_args.size())
            locate = positional_args[0];
        logfind::list_cmd(logname, locate);
        return 0;
    }

    if (cat)
    {
        if (list)
        {
            std::cerr << "--cat and --list cannot be used together" << std::endl;
            return 1;
        }
        if (!script.empty())
        {
            std::cerr << "--cat and --script cannot be used together" << std::endl;
            return 1;
        }

        if (positional_args.size() == 0)
        {
            std::cerr << "cat requires a starting time" << std::endl;
            usage();
            return 1;
        }
        
        std::string start_time;
        std::string end_time;
        int parg = 0;
        ++parg;

        if (parg >= positional_args.size())
        {
            std::cerr << "cat must have a start time" << std::endl;
            return 1;
        }
        start_time = positional_args[parg];
        ++parg;
        if (parg >= positional_args.size())
        {
            std::cerr << "cat must have a end time" << std::endl;
            return 1;
        }
        end_time = positional_args[parg];
        ++parg;

        if (logfind::cat_cmd(logname, start_time, end_time, split) == false)
        {
            std::cerr << "--cat, invalid arguments" << std::endl;
            return 1;
        }
        return 0;
    }

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
    AddPatterns(app, positional_args);

    uint64_t timestamp(0);
    bool bBefore(false);
    if (!before.empty())
    {
        bBefore = true;
        timestamp = logfind::TimespecToMicros(before);
    }
    else if (!after.empty())
    {
        bBefore = false;
        timestamp = logfind::TimespecToMicros(after);
    }
    else
    {
        bBefore = true;
        timestamp = logfind::GetCurrentTimeMicros();
    }

    std::vector<logfind::FileInfo> filesToProcess;
    if (!dash_1)
    {

        logfind::GetFilesToProcess(logname, timestamp, bBefore, filesToProcess);
    }
    else
    {
        logfind::FileInfo fi;
        fi.filepath = logname;
        filesToProcess.push_back(fi);
    }

    app.on_start();
    for (auto& fi : filesToProcess)
    {
        std::cerr << fi.filepath << std::endl;
        logfind::AhoFileContextPtr ptr = app.search();
        if (ptr->find(fi.filepath.c_str()) == false)
        {
            std::cerr << "Failed to open " << fi.filepath << std::endl;
            return 1;
        }
    }
    app.on_exit();
    return 0;
}

