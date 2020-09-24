#include <assert.h>
#include <cstdint>
#include "buffer.h"
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

    void PatternActions::on_match(Match& m)
    {
        if (disabled_)
            return;
        int fdSave = fd_;    // save
        for (Action *action : actions_)
        {
            action->pattern_actions_ = this;
            action->on_command(fd_, m);
            if (disabled_)
                return;     // we test here too since it might have just became disabled
        }
        fd_ = fdSave;
    }

    void PatternActions::on_exit(AhoContext *ctx)
    {
        for (Action *action : actions_)
        {
            action->pattern_actions_ = this;
            action->on_exit(fd_);
        }
    }
}

