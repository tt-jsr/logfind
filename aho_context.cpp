#include <assert.h>
#include <cstdint>
#include <iostream>
#include "lru_cache.h"
#include "linebuf.h"
#include "buffer.h"
#include "aho_context.h"
#include "application.h"
#include "pattern_actions.h"

int on_match(int strnum, int textpos, void *ctx);
void on_line(int textpos, void *ctx);

namespace logfind
{
    AhoLineContextPtr MakeAhoLineContext()
    {
        return std::make_shared<AhoLineContext>();
    }

    AhoFileContextPtr MakeAhoFileContext()
    {
        return std::make_shared<AhoFileContext>();
    }

    AhoContext::AhoContext()
    :acism_(nullptr)
    ,pattv_(nullptr)
    ,npatts_(0)
    {
    }

    AhoContext::~AhoContext()
    {
        acism_destroy(acism_);
        delete[] pattv_;
    }

    int AhoContext::getPatternId(const char *p)
    {
        for (size_t i = 0; i < patterns_.size(); i++)
        {
            if (strcmp(patterns_[i].c_str(), p) == 0)
                return i;
        }
        return -1;
    }

    PatternActionsPtr AhoContext::add_match_text(const char *p, uint32_t len)
    {
        auto pa = logfind::MakePatternActions();
        patterns_.push_back(std::string(p, len));
        uint32_t id = patterns_.size()-1;
        match_actions_.emplace(id, pa);        
        match_str_actions_.emplace(std::string(p, len), id);
        return pa;
    }

    PatternActionsPtr AhoContext::add_match_text(const char *p)
    {
        return add_match_text(p, strlen(p));
    }

    int AhoContext::on_match(int strnum, B_OFFSET textpos)
    {
        assert (strnum >= 0 && strnum < patterns_.size());
        textpos -= patterns_[strnum].size(); // the acism code gives us the
                                             // position at the end of the string
                                             // so we adjust
        Match m;
        m.matched_text = patterns_[strnum];
        m.searchCtx = this;
        matchData(m, textpos);

        auto it = match_actions_.find(strnum);
        assert(it != match_actions_.end());
        it->second->on_match(m);
        theApp->free(m.matched_line);
        return 0;
    }

    void AhoContext::on_exit()
    {
        for (auto& pr : match_actions_)
        {
            pr.second->on_exit(this);
        }
    }

    /**************************************************************/
    bool AhoFileContext::find(const char *fname)
    {
        bool r = file.open(fname);
        if (r == false)
        {
            return false;
        }
        lines_.clear();
        current_line_offset_ = 0;
        current_lineno_ = 0;
        current_buffer_ = nullptr;

        // The acism code needs the patterns in a MEMREF
        // structure
        if (acism_ == nullptr)
        {
            pattv_ = new MEMREF[patterns_.size()];
            npatts_ = patterns_.size();
            for (size_t i = 0; i < patterns_.size(); i++)
            {
                (pattv_[i]).ptr = patterns_[i].c_str();  // I am not copying here!
                (pattv_[i]).len = patterns_[i].size();
            }

            // Create the context
            acism_ = acism_create(pattv_, patterns_.size());
        }

        theApp->on_file_start(fname);
        int state(0);
        while (true)
        {
            if (theApp->is_exit())
                return true;
            current_buffer_ = file.get_buffer();
            if (current_buffer_ == nullptr)
            {
                //std::cerr << "buffer is null" << std::endl;
                break;
            }
            MEMREF block;
            block.ptr = current_buffer_->readPos();
            block.len = current_buffer_->availableReadBytes();

            //std::cerr << "===JSR read buffer " << current_buffer_->fileoffset() << " " << current_buffer_->availableReadBytes() << std::endl;
            (void)acism_more(acism_, block, (ACISM_ACTION*)::on_match, (ACISM_LINE*)::on_line, this, &state);
            current_buffer_->incrementReadPosition((size_t)block.len);
        }
        theApp->on_file_end(fname);
        return true;
    }

    void AhoFileContext::matchData(Match& m, B_OFFSET matchpos)
    {
        file.readLine(current_line_offset_, m.matched_line);
        m.line_offset_in_file = current_line_offset_;
        m.match_offset_in_file = matchpos + current_buffer_->fileoffset();
        m.lineno = current_lineno_;
        m.match_offset_in_line = m.match_offset_in_file - current_line_offset_;
    }

    void AhoFileContext::on_line(B_OFFSET textpos)
    {
        current_line_offset_ = current_buffer_->fileoffset() + textpos + 1;
        ++current_lineno_;
        lines_.emplace(current_lineno_, current_line_offset_);
    }

    /*********************************************************************/
    AhoLineContext::AhoLineContext()
    :matched_lineno_(0)
    {
    }

    void AhoLineContext::matchData(Match& m, B_OFFSET matchpos)
    {
        m.matched_line.buf = (char *)matched_line_.c_str();
        m.matched_line.len = matched_line_.size();
        m.matched_line.flags = LINEBUF_NONE;
        m.matched_line.bufsize = matched_line_.size();
        m.line_offset_in_file = 0;
        m.match_offset_in_file = matchpos;
        m.lineno = matched_lineno_;
        m.match_offset_in_line = matchpos;
    }
}

int on_match(int strnum, int textpos, void *v)
{
    logfind::AhoContext *ctx = (logfind::AhoContext *)v;
    return ctx->on_match(strnum, (logfind::B_OFFSET)textpos);
}

void on_line(int textpos, void *v)
{
    logfind::AhoContext *ctx = (logfind::AhoContext *)v;
    ctx->on_line((logfind::B_OFFSET)textpos);
}


