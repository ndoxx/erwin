#include <fstream>
#include <iostream>

#include "intern_string.h"
#include "debug/logger.h"

namespace erwin
{

void InternStringLocator::init(const fs::path& filepath)
{
    DLOGN("util") << "[InternStringLocator] Retrieving intern string table." << std::endl;
    std::ifstream ifs(filepath);
    parse(ifs);
}


void InternStringLocator::parse(std::istream& stream)
{
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream iss(line);
        hash_t key;
        std::string value;
        
        iss >> key >> value;
        intern_strings_.insert(std::make_pair(key,value));
    }
}


std::string InternStringLocator::operator()(hash_t hashname)
{
    auto it = intern_strings_.find(hashname);
    if(it!=intern_strings_.end())
        return it->second;
    else
        return std::string("???");
}

void InternStringLocator::add_intern_string(const std::string& str)
{
    hash_t hname = H_(str.c_str());
    if(intern_strings_.find(hname)==intern_strings_.end())
        intern_strings_.insert(std::make_pair(hname,str));
}


} // namespace erwin
