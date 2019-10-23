#pragma once

#include <cstdint>
#include <utility>

namespace erwin
{
namespace memory
{

class PoolAllocator
{
public:
	explicit PoolAllocator(std::size_t size);

	PoolAllocator(void* begin, void* end, std::size_t max_size);
	PoolAllocator(std::pair<void*,void*> ptr_range, std::size_t max_size);

	void* allocate(std::size_t size, std::size_t alignment, std::size_t offset);
	void deallocate(void* ptr);
	void reset();

private:
	uint8_t* begin_;
	uint8_t* end_;
	std::size_t max_size_;
};


} // namespace memory
} // namespace erwin