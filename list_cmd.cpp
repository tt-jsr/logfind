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
                        std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << "/" << (fi.size*20)/1000000 << " MB" << std::endl;
                    else
                        std::cout << " (" << hours << "h " << min << "m " << secs << "s) " << fi.size/1000000 << " MB" << std::endl;
                }
            }
            else
            {
                if (timestamp == 0 || timestamp > fi.timestamp)
                    std::cout << fi.filepath << std::endl << "     " << fi.sTimestamp << " " << fi.size/1000000 << " MB" << std::endl;
            }
        }
        return;
    }
}
