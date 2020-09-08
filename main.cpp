#include <stdio.h>
#include "aho_context.h"

class MyTest2 : public logfind::AhoFileContext
{
public:
    bool run()
    {
        add_match_text("bb77a423-f475-434b-a336-3bd09a32c464");

        if (open("../fsh/cme.clean") == false)
        {
            printf ("Failed to open file\n");
            return false;
        }
        start();
        return true;
    }
protected:
    void on_match(struct aho_match_t* m)
    {
        char buf[4096];
        file.readLine(m->lineno, buf, sizeof(buf));
        printf("%s\n", buf);
    }
};

class MyTest : public logfind::AhoFileContext
{
public:
    bool run()
    {
        add_match_text("S T A R T");
        add_match_text("Orderserver is ready");
        idLoadBalancer = add_match_text("loadbalancer invoked");
        add_match_text("build");

        if (open("../fsh/cme-noisy.log") == false)
        {
            printf ("Failed to open file\n");
            return false;
        }
        start();
        return true;
    }
protected:
    void on_match(struct aho_match_t* m)
    {
        if (m->id == idLoadBalancer)
        {
            MyTest2 ctx;
            ctx.run();
            return;
        }
        char buf[4096];
        file.readLine(m->lineno, buf, sizeof(buf));
        printf("%s\n", buf);
    }
    int idLoadBalancer;
};

void test()
{
    MyTest ctx;

    ctx.run();
}

int main(int /*argc*/, char ** /*argv*/)
{
    test();
    return 0;
}

