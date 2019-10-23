#include "memory/memory_utils.h"

#include <iomanip>
#include <cstdint>
#include <map>

namespace erwin
{
namespace memory
{

static std::map<uint32_t, WCB> s_highlights;

void hex_dump_highlight(uint32_t word, const WCB& wcb)
{
	s_highlights[word] = wcb;
}

void hex_dump_clear_highlights()
{
	s_highlights.clear();
}

void hex_dump(std::ostream& stream, void* ptr, std::size_t size)
{
	uint32_t* begin = static_cast<uint32_t*>(ptr);
	uint32_t* end = begin + size/4;
	uint32_t* current = begin;

	// Find 32 bytes aligned address before current if current not 32 bytes aligned itself
	std::size_t begin_offset = std::size_t(current)%32;
	current -= begin_offset/4;
	// Find offset of 32 bytes aligned address after end if end not 32 bytes aligned itself
	std::size_t end_offset = 32-std::size_t(end)%32;

	stream << WCC(102,153,0) << "/-" << WCC(0,130,10) << "HEX DUMP" << WCC(102,153,0) << "-\\" << std::endl;
	stream << std::hex;
	while(current < end+end_offset/4)
	{
		// Show 32 bytes aligned addresses
		if(std::size_t(current)%32==0)
			stream << WCC(102,153,0) << "[0x" << std::setfill('0') << std::setw(8) << std::size_t(current) << "] ";
		// Separator after 16 bytes aligned addresses
		else if(std::size_t(current)%16==0)
			stream << " ";

		// Out-of-scope data in dark gray dots
		if(current < begin || current >= end)
			stream << WCC(100,100,100) << "........";
		else
		{
			// Get current value
			uint32_t value = *current;

			// Display
			auto it = s_highlights.find(value);
			// Highlight recognized words
			if(it!=s_highlights.end())
				stream << WCC(220,220,220) << it->second << std::setfill('0') << std::setw(8) << value << WCB(0);
			// Basic display
			else
				stream << WCC(220,220,220) << std::setfill('0') << std::setw(8) << value;
		}

		// Jump lines before 32 bytes aligned addresses
		if(std::size_t(current)%32==28)
			stream << std::endl;
		else
			stream << " ";

		++current;
	}
	stream << WCC(102,153,0) << "\\-" << WCC(0,130,10) << "HEX DUMP" << WCC(102,153,0) << "-/" << std::endl;
	stream << WCC(0);
}

} // namespace memory
} // namespace erwin