#include <map>
#include <iostream>
#include <string>
#include "utilities.h"
#include "cat_cmd.h"
#include "lru_cache.h"
#include "file.h"

namespace logfind
{
    bool cat_cmd(const std::string& logname, const std::string& cat, const std::string& duration, const std::string& split)
    {
        uint64_t dur(0);
        uint64_t size = 0;
        uint64_t timestamp = TTLOG2micros(cat.c_str(), cat.size());
        if (timestamp == 0)
            return false;

        if (!duration.empty())
        {
            dur = TTLOG2micros(duration.c_str(), duration.size());
            if (dur == 0)
                return false;
        }
        if (!split.empty())
        {
            try
            {
                size = stoull(split, nullptr, 10);
            }
            catch (std::exception&)
            {
                return false;
            }
        }

        std::vector<FileInfo> files;
        GetFilesToProcess(logname, timestamp, false, files);

        for (auto& fi : files)
        {
            ReadFile f;
            if (f.open(fi.filepath.c_str()) == false)
            {
                std::cerr << "Cannot open " << fi.filepath << std::endl;
                return false;
            }
        }
        return true;
    }
}
