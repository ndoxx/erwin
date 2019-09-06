#pragma once

#include <map>

#include "../wtypes.h"
#include "../core/time_base.h"

namespace erwin
{

// WCore Console Color
// Evaluates to an ansi string when submitted to a stream
struct WCC
{
    static std::map<char, std::string> COLORMAP;

    WCC(char cc);
    WCC(uint8_t R, uint8_t G, uint8_t B);

    friend std::ostream& operator <<(std::ostream& stream, const WCC& wcc);

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

} // namespace dbg
} // namespace erwin