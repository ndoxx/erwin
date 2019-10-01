#include <fstream>
#include <iostream>
#include <unordered_map>

#include "intern_string.h"
#include "debug/logger.h"

namespace erwin
{
namespace istr
{

static std::unordered_map<hash_t, std::string> s_intern_strings;

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

void init(const fs::path& filepath)
{
    DLOGN("util") << "[InternStringLocator] Retrieving intern string table." << std::endl;
    std::ifstream ifs(filesystem::get_config_dir() / filepath);
    parse(ifs);
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
