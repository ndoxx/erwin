#pragma once

#include <map>
#include <sstream>

#include "core/core.h"
#include "core/time_base.h"

namespace erwin
{

// WCore Console Color
// Evaluates to an ansi string when submitted to a stream
struct WCC
{
    static std::map<char, std::string> COLORMAP;

    WCC() = default;
    explicit WCC(char cc);
    WCC(uint8_t R, uint8_t G, uint8_t B);

    friend std::ostream& operator <<(std::ostream& stream, const WCC& wcc);

    std::string escape;
};

// WCore Console Background color
// Evaluates to an ansi string when submitted to a stream
struct WCB
{
    static std::map<char, std::string> COLORMAP;

    WCB() = default;
    explicit WCB(int cc);
    WCB(uint8_t R, uint8_t G, uint8_t B);

    friend std::ostream& operator <<(std::ostream& stream, const WCB& wcc);

    std::string escape;
};

namespace dbg
{

enum class MsgType: std::uint8_t
{
	RAW,		// Raw message, no decoration
    NORMAL,     // No effect white message
    ITEM,       // Item in list
    EVENT,      // For event tracking
    TRACK,      // Relative to data/system being tracked NO PARSING
    NOTIFY,     // Relative to an event which should be notified to the user
    WARNING,    // Relative to an event which could impact the flow badly
    ERROR,      // Relative to a serious but recoverable error
                    // (eg. missing texture when you have a fall back one)
    FATAL,      // Relative to non recoverable error (eg. out of memory...)
    BANG,       // For code flow analysis
    GOOD,       // For test success
    BAD,        // For test fail
};

struct LogStatement
{
	hash_t channel;
	MsgType msg_type;
	TimeStamp timestamp;
	uint8_t severity;
	int code_line;
	std::string code_file;
	std::string message;
};

struct LogChannel
{
	uint8_t verbosity;
	std::string name;
	std::string tag;
};

static std::map<MsgType, WCC> STYLES =
{
    {MsgType::NORMAL,    WCC(255,255,255)},
    {MsgType::ITEM,      WCC(255,255,255)},
    {MsgType::EVENT,     WCC(255,255,255)},
    {MsgType::NOTIFY,    WCC(150,130,255)},
    {MsgType::WARNING,   WCC(255,175,0)},
    {MsgType::ERROR,     WCC(255,90, 90)},
    {MsgType::FATAL,     WCC(255,0,  0)},
    {MsgType::BANG,      WCC(255,100,0)},
    {MsgType::GOOD,      WCC(0,  255,0)},
    {MsgType::BAD,       WCC(255,0,  0)},
};

static std::map<MsgType, std::string> ICON =
{
    {MsgType::NORMAL,    "    "},
    {MsgType::ITEM,      "     \u21B3 "},
    {MsgType::EVENT,     " \u2107 "},
    {MsgType::NOTIFY,    "\033[1;48;2;20;10;50m \u2055 \033[1;49m "},
    {MsgType::WARNING,   "\033[1;48;2;50;40;10m \u203C \033[1;49m "},
    {MsgType::ERROR,     "\033[1;48;2;50;10;10m \u2020 \033[1;49m "},
    {MsgType::FATAL,     "\033[1;48;2;50;10;10m \u2021 \033[1;49m "},
    {MsgType::BANG,      "\033[1;48;2;50;40;10m \u0489 \033[1;49m "},
    {MsgType::GOOD,      "\033[1;48;2;10;50;10m \u203F \033[1;49m "},
    {MsgType::BAD,       "\033[1;48;2;50;10;10m \u2054 \033[1;49m "},
};

} // namespace dbg
} // namespace erwin