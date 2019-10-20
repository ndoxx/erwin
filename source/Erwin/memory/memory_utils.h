#pragma once

#include <cstdint>

#include "debug/logger_common.h"

namespace erwin
{
namespace memory
{
namespace utils
{

static inline std::size_t alignment_padding(const std::size_t base_address, const std::size_t alignment)
{
	const std::size_t multiplier = (base_address / alignment) + 1;
	const std::size_t aligned_address = multiplier * alignment;
	const std::size_t padding = aligned_address - base_address;
	
	return padding;
}

} // namespace utils

extern void hex_dump_highlight(uint32_t word, const WCB& wcb);
extern void hex_dump_clear_highlights();
extern void hex_dump(void* ptr, std::size_t size);

} // namespace memory
} // namespace erwin