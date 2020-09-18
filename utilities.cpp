#include <ctime>
#include <assert.h>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <ftw.h>
#include <string.h>
#include "utilities.h"
#include "lru_cache.h"
#include "file.h"
#include "application.h"
#include "linebuf.h"

namespace logfind
{
    static const int MONTHSPERYEAR = 12;

    uint64_t GetCurrentTimeMicros()
    {
        timespec ts;  
        clock_gettime(CLOCK_REALTIME, &ts); 
        return (ts.tv_sec * 1000000ULL) + (ts.tv_nsec / 1000ULL); 
    }

    time_t my_timegm(struct tm * t)
    {
        register long year;
        register time_t result;
        static const int cumdays[MONTHSPERYEAR] =
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

        year = 1900 + t->tm_year + t->tm_mon / MONTHSPERYEAR;
        result = (year - 1970) * 365 + cumdays[t->tm_mon % MONTHSPERYEAR];
        result += (year - 1968) / 4;
        result -= (year - 1900) / 100;
        result += (year - 1600) / 400;
        if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) &&
                (t->tm_mon % MONTHSPERYEAR) < 2)
            result--;
        result += t->tm_mday - 1;
        result *= 24;
        result += t->tm_hour;
        result *= 60;
        result += t->tm_min;
        result *= 60;
        result += t->tm_sec;
        if (t->tm_isdst == 1)
            result -= 3600;

        return (result);
    }

    static const uint64_t MICROS_IN_DAY = 1000000ULL * 60ULL * 60ULL * 24ULL;
    static const uint64_t MICROS_IN_HOUR = 1000000ULL * 60ULL * 60ULL;
    static const uint64_t MICROS_IN_MIN = 1000000ULL * 60ULL;
    static const uint64_t MICROS_IN_SEC = 1000000ULL;

    bool Micros2String(uint64_t micros, std::string& out)
    {
        uint64_t days = micros/MICROS_IN_DAY;
        micros -= (days * MICROS_IN_DAY);
        uint64_t hours = micros/MICROS_IN_HOUR;
        micros -= (hours * MICROS_IN_HOUR);
        uint64_t min = micros/MICROS_IN_MIN;
        micros -= (min * MICROS_IN_MIN);
        uint64_t secs = micros/MICROS_IN_SEC;
        micros -= (secs * MICROS_IN_SEC);

        char buf[64];
        sprintf (buf, "%ldd %ldh %ldm %lds %ldus", days, hours, min, secs, micros);
        out = buf;
    }

    uint64_t TTLOG2micros(const char *time, uint32_t len)
    {
        //2018-05-11 17:03:45.636730
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        tm.tm_mday = 1;

        char *p(nullptr);
        int micros = 0;
        tm.tm_year = std::strtol(time, &p, 10) - 1900;
        if (*p == '-')
        {
            ++p;
            tm.tm_mon = std::strtol(p, &p, 10) - 1;
            if (*p == '-')
            {
                ++p;
                tm.tm_mday = std::strtol(p, &p, 10);
                if (*p == ' ')
                {
                    ++p;
                    tm.tm_hour = std::strtol(p, &p, 10) - 1;
                    if (*p == ':')
                    {
                        ++p;
                        tm.tm_min = std::strtol(p, &p, 10) - 1;
                        if (*p == ':')
                        {
                            ++p;
                            tm.tm_sec = std::strtol(p, &p, 10) - 1;
                            if (*p == '.')
                            {
                                ++p;
                                micros = std::strtol(p, &p, 10);
                            }
                        }
                    }
                }
            }
        }

        time_t t = my_timegm(&tm);
        uint64_t r = (uint64_t)t * 1000000ULL;
        r += micros;
        return r;
    }

    int get_digits(const char *&p, int count)
    {
        std::string s;
        for (int i = 0; i < count; ++i)
        {
            if (std::isdigit((const unsigned char)*p) == false)
                return -1;
            s.push_back(*p);
            ++p;
        }

        return std::strtol(s.c_str(), nullptr, 10);
    }

    uint64_t TTLOGRotateTime(const std::string& filename)
    {
        //OC_cme.log-20200916-1600317601.gz
        const char *p = &filename[filename.size()-1];
        while (*p != '-' && p != &filename[0])
        {
            --p;
        }
        if (*p != '-')
            return 0;
        p += 1;
        time_t t = std::strtol(p, nullptr, 10);
        return (uint64_t)t * 1000000ULL;
    }

    uint64_t GetFirstLineTimestamp(const std::string& filename, std::string& out)
    {
        ReadFile file;
        if (file.open(filename.c_str()) == false)
            return 0;

        linebuf lb;
        if (file.readLine(0, lb) == false)
            return 0;
        
        uint64_t t = TTLOG2micros(lb.buf, lb.len);
        out.assign(lb.buf, 26);
        theApp->free(lb);
        return t;
    }

    void GetFileInfos(const std::string& logfilename, std::map<uint64_t, FileInfo>& out, bool includeTimestamp)
    {
        if (!logfilename.empty())
        {
            std::vector<std::string> filelist;
            if (GetFileList(logfilename, filelist) == false)
                std::cerr << "GetFileInfos: Unable to find logfilename '" << logfilename << std::endl;
            for (const std::string& file : filelist)
            {
                FileInfo fi;
                fi.rotatetime = TTLOGRotateTime(file);
                if (includeTimestamp)
                    fi.timestamp = GetFirstLineTimestamp(file, fi.sTimestamp);
                fi.filepath = file;
                if (fi.rotatetime == 0)
                    fi.rotatetime = GetCurrentTimeMicros();
                out.emplace(fi.rotatetime, fi);
            }
        }
    }


    std::string g_logfilename;
    std::vector<std::string> g_files;

    int ftw_cb(const char *fpath, const struct stat *sb, int typeflag)
    {
        const char *p =fpath;
        const char *s;
        while (*p)
        {
            if (*p == '/')
            {
                ++p;
                s = p;
            }
            ++p;
        }
        if (strncmp(s, g_logfilename.c_str(), g_logfilename.size()) == 0)
        {
            g_files.push_back(fpath);
        }
        return 0;
    }

    bool GetFileList(const std::string& logfilename, std::vector<std::string>& out)
    {
        g_logfilename = logfilename;
        g_files.clear();
        int r = ftw("/var/log/debesys", ftw_cb, 10);
        if (r < 0)
            std::cerr << "ftw: returned " << r << std::endl;
        out = g_files;
        return true;
    }

    uint64_t TimespecToMicros(const std::string& spec)
    {
       uint64_t microsInDay = 86400ULL * 1000000ULL;
       uint64_t microsInWeek = microsInDay * 7;
       uint64_t microsInMonth = microsInDay * 30;

       if (spec.find("day") != std::string::npos ||
               spec.find("mon") != std::string::npos ||
               spec.find("week") != std::string::npos)
       {
           size_t pos;
           try
           {
               uint64_t now = GetCurrentTimeMicros();
               int n = std::stol(spec, &pos, 10);
               ++pos;
               if (strncmp(&spec[pos], "day", 3) == 0)
                   return now - (n * microsInDay);
               else if (strncmp(&spec[pos], "week", 4) == 0)
                   return now - (n * microsInWeek);
               else if (strncmp(&spec[pos], "mon", 3) == 0)
                   return now - (n * microsInMonth);
               else
                   return 0;
           }
           catch (std::exception&)
           {
               return 0;
           }
       }

       return TTLOG2micros(spec.c_str(), spec.size());
    }
}

