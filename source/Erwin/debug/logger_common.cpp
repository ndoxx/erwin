#include <map>
#include <string>

#include "debug/logger_common.h"

namespace erwin
{

static const std::map<char, std::string> s_COLORMAP = 
{
	{0,   "\033[0m"}, // previous style
    {'p', "\033[1;38;2;0;255;255m"},   // highlight paths in light blue
    {'n', "\033[1;38;2;255;50;0m"},    // names and symbols in dark orange
    {'i', "\033[1;38;2;255;190;10m"},  // instructions in light orange
    {'w', "\033[1;38;2;220;200;255m"}, // values in light purple
    {'v', "\033[1;38;2;153;204;0m"},   // important values in green
    {'u', "\033[1;38;2;0;255;100m"},   // uniforms and attributes in light green
    {'d', "\033[1;38;2;255;100;0m"},   // default in vivid orange
    {'b', "\033[1;38;2;255;0;0m"},     // bad things in red
    {'g', "\033[1;38;2;0;255;0m"},     // good things in green
    {'z', "\033[1;38;2;255;255;255m"}, // neutral things in white
    {'x', "\033[1;38;2;0;206;209m"},   // XML nodes in turquoise
    {'h', "\033[1;38;2;255;51;204m"},  // highlight in pink
    {'s', "\033[1;38;2;0;204;153m"},   // step / phase
};

WCC::WCC(char cc):
escape(s_COLORMAP.at(cc))
{

}

WCC::WCC(uint8_t R, uint8_t G, uint8_t B)
{
	escape = "\033[1;38;2;" + std::to_string(R) + ";" + std::to_string(G) + ";" + std::to_string(B) + "m";
}


WCB::WCB(int)
{
    escape = "\033[0m";
}

WCB::WCB(uint8_t R, uint8_t G, uint8_t B)
{
    escape = "\033[1;48;2;" + std::to_string(R) + ";" + std::to_string(G) + ";" + std::to_string(B) + "m";
}

std::ostream& operator <<(std::ostream& stream, const WCC& wcc)
{
	stream << wcc.escape;
	return stream;
}

std::ostream& operator <<(std::ostream& stream, const WCB& wcb)
{
    stream << wcb.escape;
    return stream;
}

} // namespace erwin