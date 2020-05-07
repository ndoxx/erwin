#pragma once

#include <algorithm>
#include <sstream>
#include <cctype>
#include <locale>
#include <regex>
#include <functional>
#include "core/core.h"

namespace erwin
{

template<typename T>
std::string to_string(const T& x)
{
    return std::to_string(x);
}
    
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
static inline std::vector<std::string> tokenize(const std::string& str, char delimiter=',')
{
    std::vector<std::string> dst;
    std::stringstream ss(str);

    while(ss.good())
    {
        std::string substr;
        std::getline(ss, substr, delimiter);
        dst.push_back(substr);
    }
    return dst;
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
    size_t size = static_cast<size_t>(std::stoi(input.substr(0,delimiter_pos)));
    switch(H_(input.substr(delimiter_pos+1).c_str()))
    {
        case "B"_h:  return size;
        case "kB"_h: return size * 1024;
        case "MB"_h: return size * 1024*1024;
        case "GB"_h: return size * 1024*1024*1024;
    }
    return size;
}

static inline std::string size_to_string(size_t size)
{
    static const std::string sizes[] = {"_B", "_kB", "_MB", "_GB"};

    int ii = 0;
    while(size%1024 == 0 && ii < 4)
    {
        size /= 1024;
        ++ii;
    }

    return std::to_string(size) + sizes[ii];
}

static inline void center(std::string& input, int size)
{
    int diff = size - static_cast<int>(input.size());
    if(diff <= 0)
        return;

    size_t before = static_cast<size_t>(diff / 2);
    size_t after  = static_cast<size_t>(before + diff % 2);
    input = std::string(before, ' ') + input + std::string(after, ' ');
}

template <class Container>
static inline void split_string(const std::string& str, Container& cont, char delim = ' ')
{
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        cont.push_back(token);
    }
}

static inline void to_lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
}

static inline void to_upper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
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
