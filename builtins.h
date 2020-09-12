#ifndef BUILTINS_H_
#define BUILTINS_H_

namespace logfind
{
    class AhoContext;
    struct linebuf;

    struct Builtin
    {
        virtual ~Builtin() {}
        virtual bool parse(const std::vector<std::string>&) = 0;
        virtual void on_command(int fd, uint32_t lineno, linebuf& matching_line) = 0;
        virtual void on_exit(int fd) {}

        AhoContext *pCtx_;
        PatternActions *pattern_actions_;
        struct aho_match_t *aho_match_;
    };

    struct After : public Builtin
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        uint8_t lines_;
    };

    struct Before : public Builtin
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        uint8_t lines_;
    };

    struct MaxCount : public Builtin
    {
        MaxCount();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        uint32_t max_count_;
        uint32_t current_count_;
        bool exit_;
    };

    struct Print : public Builtin
    {
        Print();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;

        bool parse_line_fmt(const char *&p);
        void substitute(const char *&p, int fd, uint32_t lineno, linebuf& matchingline);
        std::string format_;
        int line_start_;
        int line_end_;
        char match_delim_;
    };

    struct LineSearch : public Builtin
    {
        LineSearch();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        void on_exit(int fd);
        PatternActionsPtr add_match_text(const char *, uint32_t len);
        PatternActionsPtr add_match_text(const char *);
        void build_trie();

        AhoLineContextPtr lineSearch_;
    };

    struct Exit : public Builtin
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
    };

    struct NamedPatternActions : public Builtin
    {
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        std::string name_;
    };

    struct File : public Builtin
    {
        File();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        std::string name_;
        bool append_;
    };

    struct Count : public Builtin
    {
        Count();
        bool parse(const std::vector<std::string>&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        void on_exit(int fd);
        uint32_t count_;
        std::string format_;
    };

    Builtin *BuiltinFactory(const std::string&);
}

#endif
