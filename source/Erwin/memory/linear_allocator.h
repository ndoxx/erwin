#pragma once

#include <cstdint>
#include <utility>

namespace erwin
{
namespace memory
{

class LinearAllocator
{
public:
	explicit LinearAllocator(std::size_t size);

	LinearAllocator(void* begin, void* end);
	LinearAllocator(std::pair<void*,void*> ptr_range);

	inline uint8_t* begin()             { return begin_; }
	inline const uint8_t* begin() const { return begin_; }
	inline uint8_t* end()               { return end_; }
	inline const uint8_t* end() const   { return end_; }

	void* allocate(std::size_t size, std::size_t alignment, std::size_t offset);
	inline void deallocate(void* ptr) { }
	inline void reset() { current_offset_ = 0; }

private:
	uint8_t* begin_;
	uint8_t* end_;
	uint32_t current_offset_;
};


} // namespace memory
} // namespace erwin