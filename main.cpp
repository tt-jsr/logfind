#include <stdio.h>
#include <iostream>
#include <map>
#include <signal.h>
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

static const char *version = ".9";

namespace logfind
{
    extern Application *theApp;
}

void usage(std::ostream& strm)
{
    const char *s = R"XXX(
logfind list logfile time [end-time]
    List the logfiles giving the starting and ending timestamps in the logs, the duration
    of the logs and compressed/decompressed file size. There's a summary printed at the end.

    logfile
        logfile will be used to generate a list of log files to be listed.
        e.g. "OC_cme.log" will include "OC_cme.log" as well as all log rotations of
        that logfile.

    time
        If a timestamp (TTLOG format: "YYYY-MM-DD hh:mm:ss") is given, it will output the
        name of the file containing that timestamp. 

    end-time  (optional)
        Stop the list at the given time.
        end-time may be "YYYY-MM-DD hh:mm:ss" or  duration "hh:mm:ss" relative to time


logfind cat logfile [-1] start-time end-time [--split size]
    Cat the contents of the log files.

    logfile
        logfile will be used to generate a list of log files to be listed.
        e.g. "OC_cme.log" will include "OC_cme.log" as well as all log rotations of
        that logfile. '-' may be used to specify stdin.

    -1  (optional)
        Interpret the logfile as the name of a log, do not include log rotations.

    start-time
        Start cat at the given time, in TTLOG format: "YYYY-MM-DD hh:mm:ss"

    end-time
        Stop the cat at the given time.
        end-time may be "YYYY-MM-DD hh:mm:ss" or  duration "hh:mm:ss" relative to start-time

    --split size
        split the output to files. The name of the files will be aa-xxxxx through zz-xxxxx.
        The size is in megabytes. Output will be written in the current directory.

logfind search logname [-1] [--script file] [--before spec] [--after spec] [pattern....]
    Search the logfiles for a list of strings. Either the script file or one or more patterns
    or both must be specified.

    logfile
        logfile will be used to generate a list of log files to be listed.
        e.g. "OC_cme.log" will include "OC_cme.log" as well as all log rotations of
        that logfile. '-' may be used to specify stdin.

    -1  (optional)
        Interpret the logfile as the name of a log, do not include log rotations.

    -s
    --script file (optional)
        The name of a script. See SCRIPT FILE below

    -b
    --before spec (optional)

    -a
    --after spec (optional)
        The before and after options indicate the starting time for the search and the
        direction. 
        Before will search backwards in the logs from the starting point, and after will search
        forward through the logs from the starting point.
        
        spec can be one of:
            "YYYY-MM-DD hh:mm:ss"    # "2020-05-04 17:30:00" - TTLOG format in UTC
            n:days                   # "4:days"              - n days ago
            n:weeks                  # "2:weeks"             - n weeks ago

    pattern... (optional)
        One or more strings to search for. Can be used in conjunction with a script file.
)XXX";
    strm << s << std::endl;
}

static void ctrl_c_handler(int signum)
{
    logfind::theApp->exit();
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

    if (logname == "-")
        dash_1 = true;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = ctrl_c_handler;
    if (sigaction (SIGINT, &act, nullptr) < 0)
    {
        std::cerr << "Failed to install sig handler" << std::endl;
        return 1;
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
        if (logname == "-")
        {
            std::cerr << "--list cannot be used with stdin" << std::endl;
            return 1;
        }
        if (dash_1)
            std::cerr << "-1 will be ignored" << std::endl;
        if (logname == "-")
        {
            std::cerr << "stdin not allowed for list" << std::endl;
            return 1;
        }
        std::string start_time;
        std::string end_time;
        int parg = 0;

        if (parg < positional_args.size())
            start_time = positional_args[parg];
        ++parg;
        if (parg < positional_args.size())
            end_time = positional_args[parg];
        logfind::list_cmd(logname, start_time, end_time);
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

    std::vector<logfind::FileInfo> filesToProcess;
    if (dash_1)
    {
        logfind::FileInfo fi;
        fi.filepath = logname;
        filesToProcess.push_back(fi);
    }
    else
    {
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
        logfind::GetFilesToProcess(logname, timestamp, bBefore, filesToProcess);
    }

    if (filesToProcess.size() == 0)
    {
        std::cerr << "Cannot find any logfiles of type " << logname << std::endl;
        return 1;
    }
    app.on_start();
    for (auto& fi : filesToProcess)
    {
        if (app.is_exit())
            break;
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

