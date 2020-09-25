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

static const char *version = ".2";

void usage(std::ostream& strm)
{
    strm << "usage: " << std::endl;
    strm << "logfind list logfile [--locate time]" << std::endl;
    strm << "    List the logfiles giving the starting and ending timestamps in the logs, the duration" << std::endl;
    strm << "    of the logs and compressed/decompressed file size" << std::endl;
    strm << "" << std::endl;
    strm << "    logfile" << std::endl;
    strm << "        logfile will be used to generate a list of log files to be listed." << std::endl;
    strm << "        e.g. \"OC_cme.log\" will include \"OC_cme.log\" as well as all log rotations of" << std::endl;
    strm << "        that logfile." << std::endl;
    strm << "" << std::endl;
    strm << "    --locate time" << std::endl;
    strm << "        If a timestamp (TTLOG format: \"YYYY-MM-DD hh:mm:ss\") is given, it will output the" << std::endl;
    strm << "        name of the file containing that timestamp." << std::endl;
    strm << "" << std::endl;
    strm << "" << std::endl;
    strm << "logfind cat logfile [-1] start-time end-time [--split size]" << std::endl;
    strm << "    Cat the contents of the log files." << std::endl;
    strm << "" << std::endl;
    strm << "    logfile" << std::endl;
    strm << "        logfile will be used to generate a list of log files to be listed." << std::endl;
    strm << "        e.g. \"OC_cme.log\" will include \"OC_cme.log\" as well as all log rotations of" << std::endl;
    strm << "        that logfile." << std::endl;
    strm << "" << std::endl;
    strm << "    -1" << std::endl;
    strm << "        Interpret the logfile as the name of a log, do not include log rotations." << std::endl;
    strm << "" << std::endl;
    strm << "    start-time" << std::endl;
    strm << "        Start cat at the given time, in TTLOG format: \"YYYY-MM-DD hh:mm:ss\"" << std::endl;
    strm << "" << std::endl;
    strm << "    end-time" << std::endl;
    strm << "        Stop the cat at the given time." << std::endl;
    strm << "        end-time may be \"YYYY-MM-DD hh:mm:ss\" or  duration \"hh:mm:ss\" relative to start-time" << std::endl;
    strm << "" << std::endl;
    strm << "    --split size" << std::endl;
    strm << "        split the output to files. The name of the files will be aa-xxxxx through zz-xxxxx." << std::endl;
    strm << "        The size is in megabytes." << std::endl;
    strm << "" << std::endl;
    strm << "logfind search logname [-1] [--script file] [--before spec] [--after spec] [pattern....]" << std::endl;
    strm << "    Search the logfiles for a list of strings. Either the script file or one or more patterns" << std::endl;
    strm << "    or both must be specified." << std::endl;
    strm << "" << std::endl;
    strm << "    logfile" << std::endl;
    strm << "        logfile will be used to generate a list of log files to be listed." << std::endl;
    strm << "        e.g. \"OC_cme.log\" will include \"OC_cme.log\" as well as all log rotations of" << std::endl;
    strm << "        that logfile." << std::endl;
    strm << "" << std::endl;
    strm << "    -1" << std::endl;
    strm << "        Interpret the logfile as the name of a log, do not include log rotations." << std::endl;
    strm << "" << std::endl;
    strm << "    -s" << std::endl;
    strm << "    --script file" << std::endl;
    strm << "        The name of a script. See SCRIPT FILE below" << std::endl;
    strm << "" << std::endl;
    strm << "    -b" << std::endl;
    strm << "    --before spec" << std::endl;
    strm << "" << std::endl;
    strm << "    -a" << std::endl;
    strm << "    --after spec" << std::endl;
    strm << "        The before and after options indicate the starting time for the search and the" << std::endl;
    strm << "        direction. " << std::endl;
    strm << "        Before will search backwards in the logs from the starting point, and after will search" << std::endl;
    strm << "        forward through the logs from the starting point." << std::endl;
    strm << "        " << std::endl;
    strm << "        spec can be one of:" << std::endl;
    strm << "            \"YYYY-MM-DD hh:mm:ss\"    # \"2020-05-04 17:30:00\" - TTLOG format in UTC" << std::endl;
    strm << "            n:days                   # \"4:days\"              - n days ago" << std::endl;
    strm << "            n:weeks                  # \"2:weeks\"             - n weeks ago" << std::endl;
    strm << "" << std::endl;
    strm << "    pattern..." << std::endl;
    strm << "        One or more strings to search for. Can be used in conjunction with a script file." << std::endl;
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
        usage(std::cout);
        return 0;
    }
    if (strcmp (argv[1], "cat") == 0)
        cat = true;
    else if (strcmp (argv[1], "list") == 0)
        list = true;
    else if (strcmp (argv[1], "search") == 0)
        search = true;
    else if (strcmp (argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
    {
        usage(std::cout);
        return 0;
    }
    else
    {
        std::cerr << "Unknown command " <<argv[1] << std::endl;
        usage(std::cerr);
        return 1;
    }

    if (argc < 3)
    {
        usage(std::cerr);
        return 0;
    }
    logname = argv[2];

    for (int a = 3; a < argc; ++a)
    {
       if (strcmp (argv[a], "--script") == 0 || strcmp(argv[a], "-s") == 0)
       {
            ++a;
            if (a == argc)
            {
                usage(std::cerr);
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
                usage(std::cerr);
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
                usage(std::cerr);
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
                usage(std::cerr);
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
                usage(std::cerr);
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
            usage(std::cerr);
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
            usage(std::cerr);
            return 1;
        }
        
        std::string start_time;
        std::string end_time;
        int parg = 0;

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
    if (dash_1)
    {
        logfind::FileInfo fi;
        fi.filepath = logname;
        filesToProcess.push_back(fi);
    }
    else
    {
        logfind::GetFilesToProcess(logname, timestamp, bBefore, filesToProcess);
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

