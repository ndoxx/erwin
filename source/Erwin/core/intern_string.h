#pragma once

#include <string>
#include <unordered_map>

#include "core/singleton.hpp"

namespace erwin
{

typedef unsigned long long hash_t;

class InternStringLocator: public Singleton<InternStringLocator>
{
public:
    friend InternStringLocator& Singleton<InternStringLocator>::Instance();
    friend void Singleton<InternStringLocator>::Kill();

    std::string operator()(hash_t hashname);
    void init(const std::string& filepath);
    void add_intern_string(const std::string& str);

private:
    void parse(std::istream& stream);

    std::unordered_map<hash_t, std::string> intern_strings_;
};


#define HRESOLVE InternStringLocator::Instance()

} // namespace erwin
