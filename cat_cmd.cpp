#include <map>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "utilities.h"
#include "cat_cmd.h"
#include "lru_cache.h"
#include "buffer.h"
#include "file.h"

namespace logfind
{
    // Return the timestamp of the last line
    // readable in this buffer
    uint64_t LastLineTimestamp(Buffer *pBuffer)
    {
        const char *p = pBuffer->writePos();
        const char *stop = pBuffer->readPos();
        uint64_t size(0);
        while (p != stop)
        {
            if (*p == '\n' && size > 26)
            {
                ++p;
                uint64_t t = TTLOG2micros(p, 26);
                if (t)
                    return t;
                --p;
            }
            --p;
            ++size;
        }
        return 0;
    }

    uint64_t FirstLineTimestamp(Buffer *pBuffer)
    {
        const char *p = pBuffer->readPos();
        const char *stop = pBuffer->writePos();
        int size(0);
        while (p != stop)
        {
            if (*p == '\n' && (pBuffer->availableReadBytes()-size) > 26)
            {
                ++p;
                uint64_t t = TTLOG2micros(p, 26);
                if (t)
                    return t;
                --p;
            }
            ++p;
            ++size;
        }
        return 0;
    }

    const char *FirstLineInBuffer(Buffer *pBuffer)
    {
        const char *p = pBuffer->readPos();
        const char *stop = pBuffer->writePos();
        while (p != stop)
        {
            if (*p == '\n')
            {
                ++p;
                return p;
            }
            ++p;
        }
        return nullptr;
    }

    int OpenSplitFile(char& c1, char &c2, const std::string& split_base_name, std::string& out)
    {
        char buf[256];
        snprintf (buf, sizeof(buf), "%c%c-%s", c1, c2, split_base_name.c_str());
        int fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ++c2;
        if (c2 > 'z')
        {
            c2 = 'a';
            ++c1;
        }
        out = buf;
        return fd;
    }

    bool cat_cmd(const std::string& logname, const std::string& start_time, const std::string& end_time, const std::string& split)
    {
        uint64_t start(0);
        uint64_t end(0);
        uint64_t split_size(0);
        uint64_t split_count(0);

        assert(!start_time.empty());
        start = TTLOG2micros(start_time.c_str(), start_time.size());
        if (start == 0)
            return false;

        assert(!end_time.empty());
        if (end_time.size() > 10)
            end = TTLOG2micros(end_time.c_str(), end_time.size());
        else
            end = HMS2micros(end_time.c_str(), end_time.size()) + start;
        if (end == start)
            return false;

        if (!split.empty())
        {
            try
            {
                split_size = stoull(split, nullptr, 10) * 1000000;
                split_count = split_size;
            }
            catch (std::exception&)
            {
                return false;
            }
        }

        std::vector<FileInfo> files;
        GetFilesToProcess(logname, start, false, files);

        int fd = 1;
        char c1 = 'a';
        char c2 = 'a';
        for (auto& fi : files)
        {
            ReadFile f;
            if (f.open(fi.filepath.c_str()) == false)
            {
                std::cerr << "Cannot open " << fi.filepath << std::endl;
                return false;
            }
            // Lets find our starting position
            Buffer *pBuffer = f.get_buffer();
            while (pBuffer)
            {
                if (LastLineTimestamp(pBuffer) > start)
                    break;
                size_t size = pBuffer->availableReadBytes();
                pBuffer->incrementReadPosition(size);
                pBuffer = f.get_buffer();
            }

            // Now start outputting
            const char *p = FirstLineInBuffer(pBuffer);
            if (p)
                pBuffer->setReadPos(p);

            size_t pos = fi.filepath.find_last_of("/");
            size_t epos = fi.filepath.find(".gz");
            if (epos == std::string::npos)
                epos = fi.filepath.size();
            std::string split_base_filename = fi.filepath.substr(pos+1, epos-(pos+1));

            while (pBuffer)
            {
                if (FirstLineTimestamp(pBuffer) > end)
                    break;
                uint32_t size = pBuffer->availableReadBytes();
                if (split_size && split_count >= split_size)
                {
                    if (fd != 1)
                        close(fd);
                    std::string fname;
                    fd = OpenSplitFile(c1, c2, split_base_filename, fname);
                    if (fd < 0)
                    {
                        std::cerr << "Unable to open " << fname << std::endl;
                        return false;
                    }
                    std::cerr << "splitting to " << fname << std::endl;
                    split_count = 0;
                }
                write(fd, pBuffer->readPos(), size);
                split_count += size;
                pBuffer->incrementReadPosition(size);
                pBuffer = f.get_buffer();
            }
        }
        if (fd != 1)
            close(fd);
        return true;
    }
}
