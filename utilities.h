#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <cstdint>
#include <time.h>
#include <string>
#include <map>
#include <vector>

namespace logfind
{
    uint64_t GetCurrentTimeMicros();

    // Convert a TLOG timestamp to microseconds
    uint64_t TTLOG2micros(const char *p, uint32_t len);
    uint64_t HMS2micros(const char *p, uint32_t len);
    std::string micros2TTLOG(uint64_t);

    void ltime(uint64_t time, int *year, int *mon, int *day, int *hours, int *min, int *secs, int *micros);

    // For the purposes of duration, a month has 30 days, a year has 365 days
    void duration(uint64_t time, int *year, int *mon, int *day, int *hours, int *min, int *secs, int *micros);

    // Convert a rotated TLOG filename into microseconds (resolution is one sec)
    uint64_t TTLOGRotateTime(const std::string&);

    // Convert microseconds into "nd nh nm ns"
    bool Micros2String(uint64_t, std::string& out);

    // In the given logfile name, return the microsecond timestamp
    // of the first log line.
    uint64_t GetFirstLineTimestamp(const std::string& filename, std::string& out);

    // parser for the --after and --before comand line options
    uint64_t TimespecToMicros(const std::string&);

    struct FileInfo
    {
        FileInfo():timestamp(0), rotatetime(0), size(0), key(0) {}
        uint64_t timestamp;      // First line in file
        std::string sTimestamp;  // First line in file
        uint64_t rotatetime;
        std::string srotatetime;
        std::string filepath;
        uint64_t size;
        uint64_t key;
    };

    // Get the list of log files in the /var/log/debesys and sub directories
    bool GetFileList(const std::string& logfilename, std::vector<FileInfo>&);

    // Get a list of file to process. The is a filter and ordered version of
    // GetFileList(), based on the timestamp and the before flag, the output
    // will be a list of files to process in the proper order
    void GetFilesToProcess(const std::string& logname, uint64_t timestamp, bool before, std::vector<FileInfo>& out);


    // Get fileinfo for logfile name
    void GetFileInfos(const std::string& logfilename, std::map<uint64_t, FileInfo>& out, bool includeTimestamp);
}

#endif

