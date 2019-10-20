#include "memory/memory_utils.h"

#include <iostream>
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

void hex_dump(void* ptr, std::size_t size)
{
	uint32_t* begin = static_cast<uint32_t*>(ptr);
	uint32_t* end = begin + size/4;
	uint32_t* current = begin;

	// Find 32 bytes aligned address before current if current not 32 bytes aligned itself
	std::size_t begin_offset = std::size_t(current)%32;
	current -= begin_offset/4;
	// Find offset of 32 bytes aligned address after end if end not 32 bytes aligned itself
	std::size_t end_offset = 32-std::size_t(end)%32;

	uint32_t fmt_cnt = 0;
	std::cout << std::hex;
	while(current < end+end_offset/4)
	{
		if(std::size_t(current)%32==0)
			std::cout << WCC(153,204,0) << std::setfill('0') << std::setw(8) << std::size_t(current) << "  ";

		uint32_t value = *current;

		// Style
		auto it = s_highlights.find(value);
		if(current < begin || current >= end)
			std::cout << WCC(100,100,100) << std::setfill('0') << std::setw(8) << value;
		else if(it!=s_highlights.end())
			std::cout << WCC(220,220,220) << it->second << std::setfill('0') << std::setw(8) << value << WCB(0);
		else
			std::cout << WCC(220,220,220) << std::setfill('0') << std::setw(8) << value;

		std::cout << " ";

		++fmt_cnt;
		if(fmt_cnt%8==0)
		{
			std::cout << std::endl;
			fmt_cnt = 0;
		}
		else if(fmt_cnt%4==0)
			std::cout << " ";

		++current;
	}
	std::cout << WCC(0) << std::endl;
}

} // namespace memory
} // namespace erwin