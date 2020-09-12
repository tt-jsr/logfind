#include <assert.h>
#include <cstdint>
#include "lru_cache.h"
#include "linebuf.h"
#include "aho_context.h"
#include "application.h"
#include "pattern_actions.h"
#include "builtins.h"

namespace logfind
{
    PatternActionsPtr MakePatternActions()
    {
        return std::make_shared<PatternActions>();
    }

    PatternActions::PatternActions()
    :fd_(1)
    , disabled_(false)
    {
    }

    PatternActions::~PatternActions()
    {
        for (auto p : commands_)
        {
            delete p;
        }
    }

    void PatternActions::disable(bool b)
    {
        disabled_ = b;
    }

    bool PatternActions::is_disabled()
    {
        return disabled_; 
    }

    void PatternActions::add_command(Builtin *bi)
    {
        commands_.push_back(bi);
    }

    void PatternActions::on_match(AhoContext *ctx, struct aho_match_t* m)
    {
        if (disabled_)
            return;
        linebuf lb;
        ctx->readLine(m->lineno, lb);
        for (Builtin *cmd : commands_)
        {
            cmd->aho_match_ = m;
            cmd->pCtx_ = ctx;
            cmd->pattern_actions_ = this;
            cmd->on_command(fd_, m->lineno, lb);
            if (disabled_)
                return;     // we test here too since it might have just became disabled
        }
        theApp->free(lb);
    }

    void PatternActions::on_exit(AhoContext *ctx)
    {
        for (Builtin *cmd : commands_)
        {
            cmd->aho_match_ = nullptr;
            cmd->pCtx_ = ctx;
            cmd->pattern_actions_ = this;
            cmd->on_exit(fd_);
        }
    }
}

