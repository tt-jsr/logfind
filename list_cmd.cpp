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
            return start > fi.timestamp && start < fi.rotatetime;
        if (start < fi.timestamp)
            return false;
        if (end >= fi.rotatetime)
            return true;
        if (end < fi.rotatetime)
            return true;
        return false;
    }

    void list_cmd(const std::string& logname, const std::string& locate)
    {
        std::map<uint64_t, FileInfo> files;
        uint64_t start(0);
        uint64_t end(0);
        uint64_t count(0);
        const FileInfo *first(nullptr);
        const FileInfo *last(nullptr);

        uint64_t totalSize(0);
        uint64_t totalUncompressed(0);


        if (!locate.empty())
        {
            uint64_t duration(0);
            start = TTLOG2micros(locate.c_str(), locate.size(), &duration);
            end = start + duration;
        }

        GetFileInfos(logname, files, true);
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
                    last = &fi;
                    totalSize += fi.size;
                    std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << " " << fi.size/1000000 << " MB" << std::endl;
                }
            }
        }

        if(files.size()>2 && first && last)
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
