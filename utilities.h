#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <cstdint>
#include <time.h>

namespace logfind
{
    uint64_t CurrentTime();
    uint64_t TTLOG_timestamp(const char *p, uint32_t len);
}

#endif

