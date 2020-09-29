#include <map>
#include <iostream>
#include <string>
#include "utilities.h"
#include "list_cmd.h"

namespace logfind
{
    bool accept(const FileInfo& fi, uint64_t start, uint64_t end)
    {
        if (start == 0)
            return true;
        if (end == 0)
            return start >= fi.timestamp && start < fi.rotatetime;
        if (start <= fi.timestamp && end > fi.rotatetime)
            return true;
        if (start >= fi.timestamp && start < fi.rotatetime)
            return true;
        if (end >= fi.timestamp && end < fi.rotatetime)
            return true;
        return false;
    }

    void list_cmd(const std::string& logname, const std::string& start_time, const std::string& end_time)
    {
        std::map<uint64_t, FileInfo> files;
        uint64_t start(0);
        uint64_t end(0);
        uint64_t count(0);
        const FileInfo *first(nullptr);
        const FileInfo *last(nullptr);

        uint64_t totalSize(0);
        uint64_t totalUncompressed(0);


        if (!start_time.empty())
        {
            start = TTLOG2micros(start_time.c_str(), start_time.size(), nullptr);
            if (start == 0)
            {
                std::cerr << "Could not parse time" << std::endl;
                return;
            }
        }

        if (!end_time.empty())
        {
            if (end_time[1] == ':' || end_time[2] == ':')
            {
                end = HMS2micros(end_time.c_str(), end_time.size());
                if (end)
                    end += start;
            }
            else
            {
                end = TTLOG2micros(end_time.c_str(), end_time.size(), nullptr);
            }
            if (end == 0)
            {
                std::cerr << "Could not parse end-time" << std::endl;
                return;
            }
        }

        if (!start_time.empty() && !end_time.empty() && (end-start == 0))
        {
            std::cerr << "Time duration between start-time and end-time is zero" << std::endl;
            return;
        }

        GetFileInfos(logname, files, true);
        if (files.size() == 0)
        {
            std::cerr << "Cannot find any logfiles of type " << logname << std::endl;
            return;
        }

        for (auto& pr : files)
        {
            const FileInfo& fi = pr.second;
            if (fi.rotatetime)
            {
                if (accept(fi, start, end))
                {
                    ++count;
                    if (!first)
                        first = &fi;
                    last = &fi;
                    uint64_t diff = fi.rotatetime - fi.timestamp;
                    int hours, min, secs;
                    duration(diff, nullptr, nullptr, nullptr, &hours, &min, &secs, nullptr);

                    std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << "  ->  " << fi.srotatetime;
                    if (fi.filepath.find(".gz") != std::string::npos)
                    {
                        totalUncompressed += fi.size*20;
                        totalSize += fi.size;
                        std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << "/" << (fi.size*20)/1000000 << " MB" << std::endl;
                    }
                    else // uncompressed rotated log
                    {
                        totalSize += fi.size;
                        std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << " MB" << std::endl;
                    }
                }
            }
            else // unrotated log
            {
                if (accept(fi, start, end))
                {
                    ++count;
                    totalSize += fi.size;
                    std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << " " << fi.size/1000000 << " MB" << std::endl;
                }
            }
        }

        if(count > 1 && files.size()>2 && first && last)
        {
            std::cout << "Summary" << std::endl;
            // The summary

            uint64_t diff = last->rotatetime - first->timestamp;
            int months, days, hours, min, secs;
            duration(diff, nullptr, &months, &days, &hours, &min, &secs, nullptr);

            const char *suffix = " MB";
            if (totalSize > 1E9)
            {
                totalSize /= 1E9;
                totalUncompressed /= 1E9;
                suffix = " GB";
            }
            else
            {
                totalSize /= 1E6;
                totalUncompressed /= 1E6;
            }
            std::cout << "    " << first->sTimestamp << "  ->  " << last->srotatetime;
            std::cout << " (" << months << "mon " << days << "days " << hours << "h " << min << "m " << secs << "s) " 
                << totalSize << "/" << totalUncompressed << suffix << " in " << count << " files" <<std::endl;
        }
        return;
    }
}
