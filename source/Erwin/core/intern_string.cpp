#include <iostream>
#include <map>

#include "intern_string.h"
#include "filesystem/filesystem.h"
#include "debug/logger.h"

namespace erwin
{
namespace istr
{

static std::map<hash_t, std::string> s_intern_strings;

static void parse(std::istream& stream)
{
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream iss(line);
        hash_t key;
        std::string value;
        
        iss >> key >> value;
        s_intern_strings.insert(std::make_pair(key, value));
    }
}

void init()
{
    DLOGN("util") << "[InternStringLocator] Retrieving intern string table." << std::endl;
    auto ifs = wfs::get_istream("syscfg://intern_strings.txt"_wp, wfs::ascii);
    parse(*ifs);
}

std::string resolve(hash_t hname)
{
    auto it = s_intern_strings.find(hname);
    if(it!=s_intern_strings.end())
        return it->second;
    else
        return std::to_string(hname);
}

void add(const std::string& str)
{
    hash_t hname = H_(str.c_str());
    if(s_intern_strings.find(hname)==s_intern_strings.end())
        s_intern_strings.insert(std::make_pair(hname,str));
}

} // namespace istr
} // namespace erwin
