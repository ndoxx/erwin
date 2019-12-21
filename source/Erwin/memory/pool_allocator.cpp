#include "memory/pool_allocator.h"
#include "memory/memory_utils.h"
#include "core/core.h"

#include <iostream>

#define ALLOCATOR_PADDING_MAGIC

namespace erwin
{
namespace memory
{

PoolAllocator::PoolAllocator(void* begin, std::size_t node_size, std::size_t max_nodes, std::size_t arena_decoration_size):
node_size_(node_size+arena_decoration_size),
max_nodes_(max_nodes),
begin_(static_cast<uint8_t*>(begin)),
end_(static_cast<uint8_t*>(begin) + max_nodes*node_size_),
free_list_(begin, node_size_, max_nodes, 0, 0)
{

}

void PoolAllocator::init(void* begin, std::size_t node_size, std::size_t max_nodes, std::size_t arena_decoration_size)
{
    node_size_ = node_size+arena_decoration_size;
    max_nodes_ = max_nodes;
    begin_ = static_cast<uint8_t*>(begin);
    end_ =static_cast<uint8_t*>(begin) + max_nodes*node_size_;
    free_list_.init(begin, node_size_, max_nodes, 0, 0);
}

void* PoolAllocator::allocate(std::size_t size, std::size_t alignment, std::size_t offset)
{
	uint8_t* next = static_cast<uint8_t*>(free_list_.acquire());
    // We want the user pointer (at next+offset) to be aligned.
    // Check if alignment is required. If so, find the next aligned memory address.
    std::size_t padding = 0;
    if(alignment && std::size_t(next+offset) % alignment)
        padding = utils::alignment_padding((std::size_t)(next+offset), alignment);

	W_ASSERT(padding + size <= node_size_, "[PoolAllocator] Allocation size does not fit initial requirement.");
	
    // Mark padding area
#ifdef ALLOCATOR_PADDING_MAGIC
    std::fill(next, next + padding, 0xd0);
#endif

    // std::cout << "Alloc: " << std::hex << size_t(next) << std::dec << " padding= " << padding << std::endl;

	return next + padding;
}

void PoolAllocator::deallocate(void* ptr)
{
	// Get unaligned address
	size_t offset = size_t((uint8_t*)ptr - begin_); // Distance in bytes to beginning of the block
	size_t padding = offset % node_size_;           // Distance in bytes to beginning of the node = padding
    // std::cout << "Dealloc: " << std::hex << size_t((uint8_t*)ptr-padding) << std::dec << " padding= " << padding << std::endl;

	free_list_.release((uint8_t*)ptr-padding);
}

void PoolAllocator::reset()
{

}

} // namespace memory
} // namespace erwin