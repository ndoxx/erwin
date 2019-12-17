#include "memory/pool_allocator.h"
#include "memory/memory_utils.h"
#include "core/core.h"

#include <iostream>

#define ALLOCATOR_PADDING_MAGIC

namespace erwin
{
namespace memory
{

PoolAllocator::PoolAllocator(void* begin, std::size_t element_size, std::size_t max_elements, std::size_t arena_decoration_size):
element_size_(element_size+arena_decoration_size),
max_elements_(max_elements),
begin_(static_cast<uint8_t*>(begin)),
end_(static_cast<uint8_t*>(begin) + max_elements*element_size_),
free_list_(begin, element_size_, max_elements, 0, 0)
{

}

void* PoolAllocator::allocate(std::size_t size, std::size_t alignment, std::size_t offset)
{
	W_ASSERT(size <= element_size_, "[PoolAllocator] Allocation size does not fit initial requirement.");
	return free_list_.acquire();
}

void PoolAllocator::deallocate(void* ptr)
{
	free_list_.release(ptr);
}

void PoolAllocator::reset()
{

}


} // namespace memory
} // namespace erwin