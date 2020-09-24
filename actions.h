#ifndef ACTIONS_H_
#define ACTIONS_H_

#include <regex>

namespace logfind
{
    class AhoContext;
    struct linebuf;
    struct Match;

    struct Action
    {
        virtual ~Action() {}
        virtual bool parse(const std::vector<std::string>&) = 0;
        virtual void on_command(int fd, Match& m) = 0;
        virtual void on_exit(int fd) {}

        PatternActions *pattern_actions_;
    };

    struct After : public Action
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        uint8_t lines_;
    };

    struct Before : public Action
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        uint8_t lines_;
    };

    struct MaxCount : public Action
    {
        MaxCount();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        uint32_t max_count_;
        uint32_t current_count_;
        bool exit_;
    };

    struct Print : public Action
    {
        Print();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;

        bool parse_line_fmt(const char *&p);
        void substitute(const char *&p, int fd, Match&);
        std::string format_;
        int line_start_;
        int line_end_;
        char match_delim_;
        int match_additional_;
    };

    struct LineSearch : public Action
    {
        LineSearch();
        ~LineSearch();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        void on_exit(int fd);
        PatternActionsPtr add_match_text(const char *, uint32_t len);
        PatternActionsPtr add_match_text(const char *);

        AhoLineContextPtr lineSearch_;
        ACISM *acism_;
        MEMREF *pattv_;
        int npatts_;
    };

    struct Exit : public Action
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
    };

    struct NamedPatternActions : public Action
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        std::string name_;
    };

    struct File : public Action
    {
        File();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        std::string name_;
        bool append_;
        bool stdout_;
        bool stderr_;
    };

    struct Count : public Action
    {
        Count();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        void on_exit(int fd);
        uint32_t count_;
        std::string format_;
        bool use_count_format_;
    };

    struct Interval : public Action
    {
        Interval();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        void on_exit(int fd);
        uint64_t lasttime_;
        std::string format_;
        bool use_time_format_;
    };

    struct Regex : public Action
    {
        Regex();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, Match&) override;
        void on_exit(int fd);

        std::regex regex_;
        std::string varname_;
    };

    Action *ActionFactory(const std::string&);
}

#endif
