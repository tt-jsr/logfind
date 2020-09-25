#ifndef CAT_CMD_H_
#define CAT_CMD_H_

namespace logfind 
{
    bool cat_cmd(const std::string& logname, const std::string& start_time, const std::string& end_time, const std::string& split);
}

#endif

