#include <map>
#include <iostream>
#include <string>
#include "utilities.h"
#include "list_cmd.h"

namespace logfind
{
    void list_cmd(const std::string& logname, const std::string& locate)
    {
        std::map<uint64_t, FileInfo> files;
        uint64_t timestamp(0);

        uint64_t totalSize(0);
        uint64_t totalUncompressed(0);


        if (!locate.empty())
        {
            timestamp = TTLOG2micros(locate.c_str(), locate.size());
        }

        GetFileInfos(logname, files, true);
        for (auto& pr : files)
        {
            const FileInfo& fi = pr.second;
            if (fi.rotatetime)
            {
                if (timestamp == 0 || (timestamp > fi.timestamp && timestamp < fi.rotatetime))
                {
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
                totalSize += fi.size;
                if (timestamp == 0 || timestamp > fi.timestamp)
                    std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << " " << fi.size/1000000 << " MB" << std::endl;
            }
        }

        if(timestamp == 0 && files.size()>2)
        {
            std::cout << "Summary" << std::endl;
            // The summary
            FileInfo& first = files.begin()->second;
            auto it = --files.end();
            if (it->second.rotatetime == 0)
                --it;
            FileInfo& last = it->second;

            uint64_t diff = last.rotatetime - first.timestamp;
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
            std::cout << "    " << first.sTimestamp << "  ->  " << last.srotatetime;
            std::cout << " (" << months << "mon " << days << "days " << hours << "h " << min << "m " << secs << "s) " 
                << totalSize << "/" << totalUncompressed << suffix << " in " << files.size() << " files" <<std::endl;
        }
        return;
    }
}
