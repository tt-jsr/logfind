#ifndef BOYER_MOORE_H__
#define BOYER_MOORE_H__

#define ALPHABET_LEN 256

struct bm_context
{
    bm_context()
    :delta2(nullptr)
    ,arg(nullptr)
    ,callback(nullptr)
    ,getchar(nullptr)
    {}

    int delta1[ALPHABET_LEN];
    int *delta2;
    void *arg;
    void (*callback)(void *arg, bm_match&);
    uint8_t (*getchar)(bm_context&);
};

struct bm_match
{
    uint64_t lineno;
    uint64_t filepos;
};

void boyer_moore_init(bm_context& ctx, uint8_t *pat, uint32_t patlen);
void boyer_moore_free(bm_context& ctx);
uint32_t boyer_moore (bm_context& ctx, uint8_t *pat, uint32_t patlen);

#endif
