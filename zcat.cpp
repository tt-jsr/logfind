#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include "buffer.h"
#include "zcat.h"

#define CHUNK 84

namespace logfind
{
    ZCat::ZCat()
    {
    }

    bool ZCat::init()
    {
        strm_.zalloc = Z_NULL;
        strm_.zfree = Z_NULL;
        strm_.opaque = Z_NULL;
        strm_.avail_in = 0;
        strm_.next_in = Z_NULL;
        ret = inflateInit(&strm_);
        if (ret != Z_OK)
        {
            return false;
        }
        return true;
    }

    int64_t ZCat::read(Buffer *buffer)
    {
        strm_.avail_out = buffer->availableWriteBytes();
        strm_.next_out = buffer->writePos();
        ret = inflate(&strm_, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
            case Z_NEED_DICT:
                std::cerr << "Need Dict error - " << std::endl;
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
                inflateEnd(&strm_);
                close(fd_);
                std::cerr << "Data Error" << std::endl;
                return ret;
            case Z_MEM_ERROR:
                inflateEnd(&strm_);
                close(fd_);
                std::cerr << "Out of memory" << std::endl;
                return ret;
        }
        have = CHUNK - strm.avail_out;
        buffer->incrementAvailableReadBytes(have);
    }

    int ZCat::read_()
    {
        int r = read(fd_, in, sizeof(in));
        if (r <= 0)
            inflateEnd(&strm_);
            close(fd_);
            return r;
        }

        strm_.avail_in = r;
        strm.next_in = in;
    }

    int zcat_default (int argcounter, char **argvector)
    {
        /* Decompress from file source to file dest until stream ends or EOF.
           inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
           allocated for processing, Z_DATA_ERROR if the deflate data is
           invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
           the version of the library linked do not match, or Z_ERRNO if there
           is an error reading or writing the files. */
        int ret;
        unsigned have;
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];

        /* allocate inflate state */
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = 0;
        strm.next_in = Z_NULL;
        ret = inflateInit(&strm);
        if (ret != Z_OK)
        {
            fclose(inFile);
            return ret;
        }

        /* decompress until deflate stream ends or end of file */
        do {
            strm.avail_in = fread(in, 1, CHUNK, inFile);
            if (ferror(inFile)) {
                (void)inflateEnd(&strm);
                fclose(inFile);
                return Z_ERRNO;
            }
            if (strm.avail_in == 0)
                break;
            strm.next_in = in;

            /* run inflate() on input until output buffer not full */
            do {
                strm.avail_out = CHUNK;
                strm.next_out = out;
                ret = inflate(&strm, Z_NO_FLUSH);
                assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                switch (ret) {
                    case Z_NEED_DICT:
                        fprintf(stderr, "Need Dict error - ");
                        ret = Z_DATA_ERROR;     /* and fall through */
                    case Z_DATA_ERROR:
                        (void)inflateEnd(&strm);
                        fclose(inFile);
                        fprintf(stderr, "Data Error\n");
                        return ret;
                    case Z_MEM_ERROR:
                        (void)inflateEnd(&strm);
                        fclose(inFile);
                        fprintf(stderr, "Out of memory\n");
                        return ret;
                }
                have = CHUNK - strm.avail_out;
                if (fwrite(out, 1, have, stdout) != have || ferror(stdout)) {
                    (void)inflateEnd(&strm);
                    fclose(inFile);
                    fprintf(stderr, "Error writing stdout\n");
                    return Z_ERRNO;
                }
            } while (strm.avail_out == 0);

            /* done when inflate() says it's done */
        } while (ret != Z_STREAM_END);

        /* clean up and return */
        (void)inflateEnd(&strm);
        fclose(inFile);
        return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
    }
}
