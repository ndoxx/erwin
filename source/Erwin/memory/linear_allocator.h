#pragma once

#include <cstdint>

namespace erwin
{

class LinearAllocator
{
public:
	explicit LinearAllocator(std::size_t size);

	LinearAllocator(void* begin, void* end);

	void* allocate(std::size_t size, std::size_t alignment, std::size_t offset);

	inline void deallocate(void* ptr) { }

	inline void reset() { current_offset_ = 0; }

private:
	uint8_t* begin_;
	uint8_t* end_;
	uint32_t current_offset_;
};


} // namespace erwin