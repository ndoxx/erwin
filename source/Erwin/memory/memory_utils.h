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

static inline std::size_t alignment_padding(std::size_t base_address, std::size_t alignment)
{
	std::size_t multiplier = (base_address / alignment) + 1;
	std::size_t aligned_address = multiplier * alignment;
	std::size_t padding = aligned_address - base_address;
	
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
	std::size_t begin;
	std::size_t end;
	std::size_t size;
};

} // namespace debug
#endif

extern void hex_dump_highlight(uint32_t word, const WCB& wcb);
extern void hex_dump_clear_highlights();
extern void hex_dump(std::ostream& stream, const void* ptr, std::size_t size, const std::string& title="");

} // namespace memory
} // namespace erwin