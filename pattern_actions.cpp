#include <assert.h>
#include <cstdint>
#include "lru_cache.h"
#include "linebuf.h"
#include "aho_context.h"
#include "application.h"
#include "pattern_actions.h"
#include "actions.h"

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
        for (auto p : actions_)
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

    void PatternActions::add_action(Action *action)
    {
        actions_.push_back(action);
    }

    void PatternActions::on_match(AhoContext *ctx, struct aho_match_t* m)
    {
        if (disabled_)
            return;
        linebuf lb;
        ctx->readLine(m->lineno, lb);
        int fdSave = fd_;    // save
        for (Action *action : actions_)
        {
            action->aho_match_ = m;
            action->pCtx_ = ctx;
            action->pattern_actions_ = this;
            action->on_command(fd_, m->lineno, lb);
            if (disabled_)
                return;     // we test here too since it might have just became disabled
        }
        fd_ = fdSave;
        theApp->free(lb);
    }

    void PatternActions::on_exit(AhoContext *ctx)
    {
        for (Action *action : actions_)
        {
            action->aho_match_ = nullptr;
            action->pCtx_ = ctx;
            action->pattern_actions_ = this;
            action->on_exit(fd_);
        }
    }
}

