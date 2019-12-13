#pragma once

#include <algorithm>
#include <sstream>
#include <cctype>
#include <locale>
#include <regex>
#include <functional>
#include "core/wtypes.h"

namespace erwin
{
namespace su
{
// Trim from start (in place)
extern inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
    {
        return !std::isspace(ch);
    }));
}

// Trim from end (in place)
extern inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return !std::isspace(ch);
    }).base(), s.end());
}

// Trim from both ends (in place)
extern inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// Trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// Trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// Trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

// Tokenize an input string into a vector of strings, specifying a delimiter
static inline void tokenize(const std::string& str, std::vector<std::string>& dst, char delimiter=',')
{
    std::stringstream ss(str);

    while(ss.good())
    {
        std::string substr;
        std::getline(ss, substr, delimiter);
        dst.push_back(substr);
    }
}

// Tokenize an input string and call a visitor for each token
static inline void tokenize(const std::string& str, char delimiter, std::function<void(const std::string&)> visit)
{
    std::stringstream ss(str);

    while(ss.good())
    {
        std::string substr;
        std::getline(ss, substr, delimiter);
        visit(substr);
    }
}

// Convert a size string to a number
static inline size_t parse_size(const std::string& input, char delimiter='_')
{
    auto delimiter_pos = input.find_first_of(delimiter);
    size_t size = std::stoi(input.substr(0,delimiter_pos));
    switch(H_(input.substr(delimiter_pos+1).c_str()))
    {
        case "B"_h:  return size;
        case "kB"_h: return size * 1024;
        case "MB"_h: return size * 1024*1024;
        case "GB"_h: return size * 1024*1024*1024;
    }
    return size;
}

namespace rx
{
// Regex replace with a callback
template<class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
    const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
    std::basic_string<CharT> s;

    typename std::match_results<BidirIt>::difference_type
        positionOfLastMatch = 0;
    auto endOfLastMatch = first;

    auto callback = [&](const std::match_results<BidirIt>& match)
    {
        auto positionOfThisMatch = match.position(0);
        auto diff = positionOfThisMatch - positionOfLastMatch;

        auto startOfThisMatch = endOfLastMatch;
        std::advance(startOfThisMatch, diff);

        s.append(endOfLastMatch, startOfThisMatch);
        s.append(f(match));

        auto lengthOfMatch = match.length(0);

        positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

        endOfLastMatch = startOfThisMatch;
        std::advance(endOfLastMatch, lengthOfMatch);
    };

    std::regex_iterator<BidirIt> begin(first, last, re), end;
    std::for_each(begin, end, callback);

    s.append(endOfLastMatch, last);

    return s;
}

template<class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string& s,
    const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
    return regex_replace(s.cbegin(), s.cend(), re, f);
}

} // namespace rx
} // namespace su
} // namespace erwin
