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
    std::cerr << "\
       usage: logfind pattern ... searchFile \
             Find patterns in the given searchFile \
\
       logfind --script file --logname name [--after spec][--before spec] [pattern pattern...] \
             Run a script for pattern matching. \
             --logname: The name of the logfile (e.g. OC_cme.log). This will search all logs in. \
                        /var/log/debesys and /var/log/debesys/oldlogs in chronological order. \
             --after spec: Provide a starting time and search logs after this date-time. \
             --before spec: Provide a starting time and search logs before this date-time. \
                            spec is TTLOG format \"YYYY-MM-DD hh:mm:ss\" \
                                    n:days \
                                    n:weeks \
\
       logfind --list --logname name [--locate datetime] \
             List the log files. \
             --locate: locate the logfile that contains the datetime in TTLOG format \"YYYY-MM-DD hh:mm:ss\". \
\
       logfind --cat datetime --logname name [--duration hh:mm::s][--split n] \
             Concatenate a time range of the logfiles. \
             --duration: The time rangle of logs to output, in hh:mm:ss format. \
             --split size: Split the files in size MB chunks. \
                           Filenames will be aa-filename through zz-filename. \
\
       logfind --version \
";
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
    std::string script;
    std::string infile;
    std::vector<std::string> nodash;
    std::string logname;
    std::string before;
    std::string after;
    std::string locate;
    std::string cat;
    std::string duration;
    std::string split;
    uint64_t timestamp(0);
    bool list(false);

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
        else if (strcmp (argv[a], "--logname") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--logname requires filename" << std::endl;
                return 1;
            }
            logname = argv[a];
        }
        else if (strcmp (argv[a], "--cat") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--cat requires filename" << std::endl;
                return 1;
            }
            cat = argv[a];
        }
        else if (strcmp (argv[a], "--duration") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--duration requires filename" << std::endl;
                return 1;
            }
            duration = argv[a];
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
        else if (strcmp (argv[a], "--before") == 0)
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
        else if (strcmp (argv[a], "--after") == 0)
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
        else if (strcmp (argv[a], "--list") == 0)
        {
            list = true;
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

    if (list)
    {
        if (!cat.empty())
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
        logfind::list_cmd(logname, locate);
        return 0;
    }

    if (!cat.empty())
    {
        if (logname.empty())
        {
            std::cerr << "--cat requires --logname" << std::endl;
            return 1;
        }
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
        if (logfind::cat_cmd(logname, cat, duration, split) == false)
        {
            std::cerr << "--cat, invalid arguments" << std::endl;
            return 1;
        }
        return 0;
    }

    if ((!before.empty() || !after.empty()) && logname.empty())
    {
        std::cerr << "--before or --after requires --logname" << std::endl;
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

    std::vector<logfind::FileInfo> filesToProcess;
    if (!logname.empty())
    {
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
    else
    {
        logfind::FileInfo fi;
        fi.filepath = infile;
        filesToProcess.push_back(fi);
    }

    //for (auto& fi : filesToProcess)
    //{
        //std::cerr << "===JSR " << fi.filepath << std::endl;
    //}

    // let'r rip...
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

