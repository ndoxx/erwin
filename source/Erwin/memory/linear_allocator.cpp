#include "memory/linear_allocator.h"
#include "memory/memory_utils.h"
#include "core/core.h"

#include <iostream>

#define ALLOCATOR_PADDING_MAGIC

namespace erwin
{
namespace memory
{

LinearAllocator::LinearAllocator(std::size_t size)
{

}

LinearAllocator::LinearAllocator(void* begin, void* end):
begin_(static_cast<uint8_t*>(begin)),
end_(static_cast<uint8_t*>(end)),
current_offset_(0)
{

}

LinearAllocator::LinearAllocator(std::pair<void*,void*> ptr_range):
LinearAllocator(ptr_range.first, ptr_range.second)
{

}

void* LinearAllocator::allocate(std::size_t size, std::size_t alignment, std::size_t offset)
{
	uint8_t* current = begin_ + current_offset_;

    // We want the user pointer (at current+offset) to be aligned.
    // Check if alignment is required. If so, find the next aligned memory address.
    std::size_t padding = 0;
    if(alignment && std::size_t(current+offset) % alignment)
        padding = utils::alignment_padding((std::size_t)(current+offset), alignment);

	// Out of memory
    if(current + padding + size > end_)
    {
    	W_ASSERT(false, "[LinearAllocator] Out of memory!");
        return nullptr;
    }

    // Mark padding area
#ifdef ALLOCATOR_PADDING_MAGIC
    std::fill(current, current + padding, 0xd0);
#endif

    current_offset_ += padding + size;
    return current + padding;
}

} // namespace memory
} // namespace erwin