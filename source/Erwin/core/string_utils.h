#pragma once

#include <algorithm>
#include <cctype>
#include <locale>
#include <regex>

namespace erwin
{

// trim from start (in place)
extern inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
    {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
extern inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
    {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
extern inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
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
} // namespace erwin
