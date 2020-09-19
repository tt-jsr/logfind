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
    std::cout << "usage: logfind [--script file] pattern ... searchFile" << std::endl;
    std::cout << "       logfind [--script file][--logname name][--after spec][--before spec] [pattern pattern...] file" << std::endl;
    std::cout << "       logfind --logname name --list" << std::endl;
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
    std::string logname;
    std::string timespec;
    uint64_t timestamp(0);
    bool before(false);
    bool after(false);
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
        else if (strcmp (argv[a], "--before") == 0)
        {
            ++a;
            if (a == argc)
            {
                usage();
                std::cout << "--before requires time spec" << std::endl;
                return 1;
            }
            timespec = argv[a];
            before = true;
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
            timespec = argv[a];
            after = true;
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
        if (logname.empty())
        {
            std::cerr << "--list requires --logname" << std::endl;
            return 1;
        }
        std::map<uint64_t, logfind::FileInfo> files;

        logfind::GetFileInfos(logname, files, true);
        for (auto& pr : files)
        {
            const logfind::FileInfo& fi = pr.second;
            if (fi.rotatetime)
            {
                uint64_t diff = fi.rotatetime - fi.timestamp;
                int hours, min, secs;
                logfind::duration(diff, nullptr, nullptr, nullptr, &hours, &min, &secs, nullptr);

                std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << "  ->  " << fi.srotatetime;
                if (fi.filepath.find(".gz") != std::string::npos)
                    std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << "/" << (fi.size*20)/1000000 << " MB" << std::endl;
                else
                    std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << " MB" << std::endl;
            }
            else
            {
                std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << " " << fi.size/1000000 << " MB" << std::endl;
            }
        }
        return 0;
    }

    if (!timespec.empty() && logname.empty())
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
        if (before || after)
        {
            timestamp = logfind::TimespecToMicros(timespec);
        }
        else
        {
            before = true;
            timestamp = logfind::GetCurrentTimeMicros();
        }

        std::map<uint64_t, logfind::FileInfo> files;

        logfind::GetFileInfos(logname, files, false);

        // search the logrotate time until our timestamp
        // is later
        uint64_t key;
        for (auto it = files.begin(); it != files.end(); ++it)
        {
            if (it->second.key > timestamp)
            {
                key = it->first;
                ++it;
                if (it != files.end())
                {
                    // Lets make sure our timestamp isn't in the next file
                    std::string s;
                    uint64_t t = logfind::GetFirstLineTimestamp(it->second.filepath, s);
                    if (t < timestamp)
                        key = it->first;
                }
                break;
            }
        }
        // we have our starting file, collect the files we need to process
        if (after)
        {
            auto it = files.find(key);
            while (it != files.end())
            {
                filesToProcess.push_back(it->second);
                ++it;
            }
        }
        else
        {
            auto it = files.find(key);
            while (it != files.begin())
            {
                filesToProcess.push_back(it->second);
                --it;
            }
            filesToProcess.push_back(files.begin()->second);
        }
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

