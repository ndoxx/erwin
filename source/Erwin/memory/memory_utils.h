#pragma once

#include <cstdint>
#include <string>
#include <ostream>

#include "debug/logger_common.h"

namespace erwin
{
namespace memory
{
namespace utils
{

static inline std::size_t alignment_padding(uint8_t* base_address, std::size_t alignment)
{
	size_t base_address_sz = reinterpret_cast<std::size_t>(base_address);

	std::size_t multiplier = (base_address_sz / alignment) + 1;
	std::size_t aligned_address = multiplier * alignment;
	std::size_t padding = aligned_address - base_address_sz;
	
	return padding;
}

// Return a human readable size string
extern std::string human_size(std::size_t bytes);

} // namespace utils

#ifdef W_DEBUG
namespace debug
{

struct AreaItem
{
	std::string name;
	void* begin = nullptr;
	void* end = nullptr;
	std::size_t size = 0;
};

} // namespace debug
#endif

extern void hex_dump_highlight(uint32_t word, const WCB& wcb);
extern void hex_dump_clear_highlights();
extern void hex_dump(std::ostream& stream, const void* ptr, std::size_t size, const std::string& title="");

} // namespace memory
} // namespace erwin