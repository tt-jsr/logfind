#include <stdio.h>
#include "file.h"

extern "C"
{
#include "ahocorasick.h"
}

extern "C" void callback_match_total(void *arg, struct aho_match_t* m)
{
    long long int* match_total = (long long int*)arg;
    (*match_total)++;
}

extern "C" void callback_match_pos(void *arg, struct aho_match_t* m)
{
    logfind::ReadFile *pFile = (logfind::ReadFile *)arg;

    //printf("match id: %d position: %llu length: %d lineno: %llu linepos: %llu\n", m->id, m->pos, m->len, m->lineno, m->lineoff);
    char buf[4096];
    pFile->readLine(m->lineno, buf, sizeof(buf));
    printf("%s\n", buf);
}

extern "C" char lf_getchar(void *arg)
{
    logfind::ReadFile *pFile = (logfind::ReadFile *)arg;
    return pFile->get();
}

void test()
{
    struct ahocorasick aho;
    long long int match_total = 0;
    int id[10] = {0};

    aho_init(&aho);

    id[0] = aho_add_match_text(&aho, "S T A R T", 9);
    id[1] = aho_add_match_text(&aho, "Orderserver is ready", 19);
    id[2] = aho_add_match_text(&aho, "loadbalancer invoked", 19);
    id[3] = aho_add_match_text(&aho, "build", 5);

    logfind::ReadFile file;
    if (file.open("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
        return;
    }
    aho_create_trie(&aho);
    aho_register_match_callback(&aho, callback_match_pos, &file);

    aho_findtext(&aho, lf_getchar, &file);

    aho_destroy(&aho);
}

int main(int /*argc*/, char ** /*argv*/)
{
    test();
    return 0;
}
