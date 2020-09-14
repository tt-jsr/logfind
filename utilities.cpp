#include <ctime>
#include <assert.h>
#include <cstdlib>
#include "utilities.h"

namespace logfind
{
    static const int MONTHSPERYEAR = 12;

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
        if (len < 26)
            return 0;
        //2018-05-11 17:03:45.636730
        struct tm tm;
        char *p(nullptr);
        tm.tm_year = std::strtol(time, &p, 10) - 1900;
        assert(*p == '-');
        ++p;
        tm.tm_mon = std::strtol(p, &p, 10) - 1;
        assert(*p == '-');
        ++p;
        tm.tm_mday = std::strtol(p, &p, 10);
        assert(*p == ' ');
        ++p;
        tm.tm_hour = std::strtol(p, &p, 10) - 1;
        assert(*p == ':');
        ++p;
        tm.tm_min = std::strtol(p, &p, 10) - 1;
        assert(*p == ':');
        ++p;
        tm.tm_sec = std::strtol(p, &p, 10) - 1;
        assert(*p == '.');
        ++p;
        int micros = std::strtol(p, &p, 10);

        time_t t = my_timegm(&tm);
        uint64_t r = (uint64_t)t * 1000000;
        r += micros;
        return r;
    }
}

