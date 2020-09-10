#ifndef BUILTINS_H_
#define BUILTINS_H_

namespace logfind
{
    class AhoContext;
    struct linebuf;

    struct Builtin
    {
        virtual ~Builtin() {}
        virtual void parse(const std::string&) = 0;
        virtual void on_command(int fd, uint32_t lineno, linebuf& matching_line) = 0;

        AhoContext *pCtx_;
        PatternActions *pattern_actions_;
        struct aho_match_t *aho_match_;
    };

    struct After : public Builtin
    {
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        uint8_t lines_;
    };

    struct Before : public Builtin
    {
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        uint8_t lines_;
    };

    struct Print : public Builtin
    {
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        std::string format_;
    };

    struct LineSearch : public Builtin
    {
        LineSearch();
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        PatternActionsPtr add_match_text(const char *, uint32_t len);
        PatternActionsPtr add_match_text(const char *);
        void build_trie();

        AhoLineContextPtr lineSearch_;
    };

    struct Exit : public Builtin
    {
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
    };

    struct NamedPatternActions : public Builtin
    {
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        std::string name_;
    };

    struct File : public Builtin
    {
        File();
        void parse(const std::string&) override;
        void on_command(int fd, uint32_t lineno, linebuf& matching_line) override;
        std::string name_;
        bool append_;
    };

    Builtin *BuiltinFactory(const std::string&);
}

#endif
