#include <ctime>
#include <assert.h>
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <ftw.h>
#include <string.h>
#include "buffer.h"
#include "utilities.h"
#include "lru_cache.h"
#include "file.h"
#include "application.h"
#include "linebuf.h"

namespace logfind
{
    static const int MONTHSPERYEAR = 12;
    static const uint64_t MICROS_IN_SEC = 1000000ULL;
    static const uint64_t MICROS_IN_MIN = MICROS_IN_SEC * 60;
    static const uint64_t MICROS_IN_HOUR = MICROS_IN_MIN * 60;
    static const uint64_t MICROS_IN_DAY = MICROS_IN_HOUR * 24;
    static const uint64_t MICROS_IN_MONTH = MICROS_IN_DAY * 30;
    static const uint64_t MICROS_IN_YEAR = MICROS_IN_MONTH * 365;

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

    void ltime(uint64_t time, int *year, int *month, int *day, int *hours, int *min, int *secs, int *micros)
    {
        time_t t = time/1000000;
        int us = time - (t * 1000000);
        struct tm *tm = gmtime(&t);
        if (year)
            *year = tm->tm_year +1900;
        if (month)
            *month = tm->tm_mon + 1;
        if (day)
            *day = tm->tm_mday;
        if (min)
            *min = tm->tm_min;
        if (secs)
            *secs = tm->tm_sec;
        if (micros)
            *micros = us;
    }

    void duration(uint64_t time, int *year, int *month, int *day, int *hours, int *min, int *secs, int *micros)
    {
        if (year)
        {
            int y = time/MICROS_IN_YEAR;
            time -= y * MICROS_IN_YEAR;
            *year = y;
        }
        if (month)
        {
            int mon = time/MICROS_IN_MONTH;
            time -= mon * MICROS_IN_MONTH;
            *month = mon;
        }
        if(day)
        {
            int d = time/MICROS_IN_DAY;
            time -= d * MICROS_IN_DAY;
            *day = d;
        }
        if (hours)
        {
            int h = time/MICROS_IN_HOUR;
            time -= h * MICROS_IN_HOUR;
            *hours = h;
        }
        if (min)
        {
            int m = time/MICROS_IN_MIN;
            time -= m * MICROS_IN_MIN;
            *min = m;
        }
        if (secs)
        {
            int s = time/MICROS_IN_SEC;
            time -= s * MICROS_IN_SEC;
            *secs = s;
        }
        if (micros)
        {
            *micros = time;
        }
    }

    uint64_t TTLOG2micros(const char *time, uint32_t len, uint64_t *durationOut)
    {
        //2018-05-11 17:03:45.636730
        struct tm tm;
        memset(&tm, 0, sizeof(tm));
        tm.tm_mday = 1;

        // very basic validation
        if (*(time+4) != '-' || *(time+7) != '-' )
            return 0;

        uint64_t duration(0);

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
                    tm.tm_hour = std::strtol(p, &p, 10);
                    if (*p == ':')
                    {
                        ++p;
                        tm.tm_min = std::strtol(p, &p, 10);
                        if (*p == ':')
                        {
                            ++p;
                            tm.tm_sec = std::strtol(p, &p, 10);
                            if (*p == '.')
                            {
                                ++p;
                                micros = std::strtol(p, &p, 10);
                            }
                            else
                                duration = MICROS_IN_SEC;
                        }
                        else
                            duration = MICROS_IN_MIN;
                    }
                    else
                        duration = MICROS_IN_HOUR;
                }
                else
                    duration = MICROS_IN_DAY;
            }
            else
                duration = MICROS_IN_MONTH;
        }

        time_t t = my_timegm(&tm);
        uint64_t r = (uint64_t)t * 1000000ULL;
        r += micros;
        if (durationOut)
            *durationOut = duration;
        return r;
    }

    uint64_t HMS2micros(const char *time, uint32_t len)
    {
        //17:03:45

        uint64_t hour(0);
        uint64_t min(0);
        uint64_t sec(0);

        char *p(nullptr);
        hour = std::strtol(time, &p, 10);
        hour *= MICROS_IN_HOUR;
        if (*p == ':')
        {
            ++p;
            min = std::strtol(p, &p, 10);
            min *= MICROS_IN_MIN;
            if (*p == ':')
            {
                ++p;
                sec = std::strtol(p, &p, 10);
                sec *= MICROS_IN_SEC;
            }
        }

        return hour + min + sec;
    }

    std::string micros2TTLOG(uint64_t micros, bool bPrintMicro)
    {
        time_t t = micros/1000000;
        micros = micros - (t*1000000);
        struct tm *tm = gmtime(&t);
        char buf[64];
        if (bPrintMicro)
            sprintf(buf, "%02d-%02d-%02d %02d:%02d:%02d.%04d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, micros);
        else
            sprintf(buf, "%02d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        return std::string(buf);
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
        
        uint64_t t = TTLOG2micros(lb.buf, lb.len, nullptr);
        out.assign(lb.buf, 26);
        theApp->free(lb);
        return t;
    }

    void GetFileInfos(const std::string& logfilename, std::map<uint64_t, FileInfo>& out, bool includeTimestamp)
    {
        if (!logfilename.empty())
        {
            std::vector<FileInfo> filelist;
            if (GetFileList(logfilename, filelist) == false)
                std::cerr << "GetFileInfos: Unable to find logfilename '" << logfilename << std::endl;
            for (FileInfo& fi : filelist)
            {
                if (includeTimestamp)
                    fi.timestamp = GetFirstLineTimestamp(fi.filepath, fi.sTimestamp);
                fi.key = fi.rotatetime;
                if (fi.rotatetime == 0)
                    fi.key = GetCurrentTimeMicros();
                out.emplace(fi.key, fi);
            }
        }
    }


    std::string g_logfilename;
    std::vector<FileInfo> g_files;

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
            FileInfo fi;
            fi.filepath = fpath;
            fi.size = sb->st_size;
            fi.rotatetime = TTLOGRotateTime(fi.filepath);
            fi.srotatetime = micros2TTLOG(fi.rotatetime, false);
            g_files.push_back(fi);
        }
        return 0;
    }

    bool GetFileList(const std::string& logfilename, std::vector<FileInfo>& out)
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

       return TTLOG2micros(spec.c_str(), spec.size(), nullptr);
    }

    void GetFilesToProcess(const std::string& logname, uint64_t timestamp, bool before, std::vector<FileInfo>& out)
    {
        std::map<uint64_t, FileInfo> files;

        GetFileInfos(logname, files, false);

        if (timestamp == 0)
        {
            auto it = files.begin();
            while (it != files.end())
            {
                out.push_back(it->second);
                ++it;
            }
            return;
        }

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
                    uint64_t t = GetFirstLineTimestamp(it->second.filepath, s);
                    if (t < timestamp)
                        key = it->first;
                }
                break;
            }
        }
        // we have our starting file, collect the files we need to process
        if (!before)
        {
            auto it = files.find(key);
            while (it != files.end())
            {
                out.push_back(it->second);
                ++it;
            }
        }
        else
        {
            auto it = files.find(key);
            while (it != files.begin())
            {
                out.push_back(it->second);
                --it;
            }
            out.push_back(files.begin()->second);
        }
    }
}

