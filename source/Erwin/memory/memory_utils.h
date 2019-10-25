#pragma once

#include <cstdint>
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

} // namespace utils

extern void hex_dump_highlight(uint32_t word, const WCB& wcb);
extern void hex_dump_clear_highlights();
extern void hex_dump(std::ostream& stream, const void* ptr, std::size_t size);

} // namespace memory
} // namespace erwin