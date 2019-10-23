#include "memory/pool_allocator.h"
#include "memory/memory_utils.h"
#include "core/core.h"

#include <iostream>

#define ALLOCATOR_PADDING_MAGIC

namespace erwin
{
namespace memory
{

PoolAllocator::PoolAllocator(std::size_t size)
{

}

PoolAllocator::PoolAllocator(void* begin, void* end, std::size_t max_size):
begin_(static_cast<uint8_t*>(begin)),
end_(static_cast<uint8_t*>(end)),
max_size_(max_size)
{

}

PoolAllocator::PoolAllocator(std::pair<void*,void*> ptr_range, std::size_t max_size):
PoolAllocator(ptr_range.first, ptr_range.second, max_size)
{

}

void* PoolAllocator::allocate(std::size_t size, std::size_t alignment, std::size_t offset)
{
    return nullptr;
}

void PoolAllocator::deallocate(void* ptr)
{

}

void PoolAllocator::reset()
{

}


} // namespace memory
} // namespace erwin