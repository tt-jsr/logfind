#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <cstdint>
#include <time.h>
#include <string>

namespace logfind
{
    uint64_t CurrentTime();
    uint64_t TTLOG2micros(const char *p, uint32_t len);
    bool Micros2String(uint64_t, std::string& out);
}

#endif

