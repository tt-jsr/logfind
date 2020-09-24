#ifndef PATTERN_ACTIONS_H_
#define PATTERN_ACTIONS_H_

#include <cstdint>
#include <unordered_map>
#include <string.h>
#include <vector>
#include <memory>
#include "file.h"

namespace logfind
{
    class AhoLineContext;
    class PatternActions;
    class Action;
    struct Match;

    using PatternActionsPtr = std::shared_ptr<PatternActions>;
    using AhoLineContextPtr = std::shared_ptr<AhoLineContext>;

    class PatternActions
    {
    public:
        PatternActions();
        ~PatternActions();
        void on_match(Match&);
        void on_exit(AhoContext *ctx);
        void add_action(Action *);
        void disable(bool);
        bool is_disabled();
    private:
        friend class File;
        int fd_;
        std::vector<Action *> actions_;
        bool disabled_;
    };

    PatternActionsPtr MakePatternActions();
}

#endif

