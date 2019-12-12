#pragma once

#include <cstdint>
#include <utility>
#include "memory/free_list.h"

namespace erwin
{
namespace memory
{

class PoolAllocator
{
public:
	PoolAllocator(void* begin, std::size_t element_size, std::size_t max_elements, std::size_t arena_decoration_size);

	inline uint8_t* begin()             { return begin_; }
	inline const uint8_t* begin() const { return begin_; }
	inline uint8_t* end()               { return end_; }
	inline const uint8_t* end() const   { return end_; }

	void* allocate(std::size_t size, std::size_t alignment=0, std::size_t offset=0);
	void deallocate(void* ptr);
	void reset();

private:
	std::size_t element_size_;
	std::size_t max_elements_;
	uint8_t* begin_;
	uint8_t* end_;
	Freelist free_list_;
};


} // namespace memory
} // namespace erwin