#pragma once

#include <cstdint>

namespace erwin
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
} // namespace erwin