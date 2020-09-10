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
    class Builtin;

    using PatternActionsPtr = std::shared_ptr<PatternActions>;
    using AhoLineContextPtr = std::shared_ptr<AhoLineContext>;

    class PatternActions
    {
    public:
        PatternActions();
        ~PatternActions();
        void on_match(AhoContext *ctx, struct aho_match_t* m);
        void add_command(Builtin *);
    private:
        friend class File;
        int fd_;
        std::vector<Builtin *> commands_;
    };

    PatternActionsPtr MakePatternActions();
}

#endif

