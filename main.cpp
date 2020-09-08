#include <stdio.h>
#include "aho_context.h"

class MyTest : public logfind::AhoFileContext
{
protected:
    void on_match(struct aho_match_t* m)
    {
        char buf[4096];
        file.readLine(m->lineno, buf, sizeof(buf));
        printf("%s\n", buf);
    }
};

void test()
{
    MyTest ctx;

    ctx.add_match_text("S T A R T");
    ctx.add_match_text("Orderserver is ready");
    ctx.add_match_text("loadbalancer invoked");
    ctx.add_match_text("build");

    if (ctx.open("../fsh/cme-noisy.log") == false)
    {
        printf ("Failed to open file\n");
        return;
    }

    ctx.start();
}

/*
void test2()
{
    struct ahocorasick aho;
    long long int match_total = 0;
    int id[10] = {0};

    aho_init(&aho);

    id[0] = aho_add_match_text(&aho, "bb77a423-f475-434b-a336-3bd09a32c464", 35);

    logfind::ReadFile file;
    if (file.open("../fsh/cme.clean") == false)
    {
        printf ("Failed to open file\n");
        return;
    }
    aho_create_trie(&aho);
    aho_register_match_callback(&aho, callback_match_pos, &file);

    aho_findtext(&aho, lf_getchar, &file);

    aho_destroy(&aho);
}
*/
int main(int /*argc*/, char ** /*argv*/)
{
    test();
    return 0;
}

