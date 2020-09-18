#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <cstdint>
#include <time.h>
#include <string>
#include <map>
#include <vector>

namespace logfind
{
    uint64_t CurrentTime();
    // Convert a TLOG timestamp to microseconds
    uint64_t TTLOG2micros(const char *p, uint32_t len);

    // Convert a rotated TLOG filename into microseconds (resolution is one sec)
    uint64_t TTLOGRotateTime(const std::string&);

    // Convert microseconds into "nd nh nm ns"
    bool Micros2String(uint64_t, std::string& out);

    // In the given logfile name, return the microsecond timestamp
    // of the first log line.
    uint64_t GetFirstLineTimestamp(const std::string& filename, std::string& out);

    // Get the list of log files in the /var/log/debesys and sub directories
    bool GetFileList(const std::string& logfilename, std::vector<std::string>&);

    struct FileInfo
    {
        uint64_t timestamp;
        std::string sTimestamp;
        uint64_t rotatetime;
        std::string filepath;
    };

    // Get fileinfo for logfile name
    void GetFileInfos(const std::string& logfilename, std::map<uint64_t, FileInfo>& out);
}

#endif

