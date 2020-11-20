#pragma once

#include <kibble/memory/memory.inc>
#include <kibble/memory/linear_allocator.h>
#include <kibble/memory/free_list.h>
#include <kibble/memory/pool_allocator.h>

namespace erwin
{

#ifdef ARENA_RETAIL
typedef kb::memory::MemoryArena<kb::memory::LinearAllocator, kb::memory::policy::SingleThread,
                                kb::memory::policy::NoBoundsChecking, kb::memory::policy::NoMemoryTagging,
                                kb::memory::policy::NoMemoryTracking>
    LinearArena;

typedef kb::memory::MemoryArena<kb::memory::PoolAllocator, kb::memory::policy::SingleThread,
                                kb::memory::policy::NoBoundsChecking, kb::memory::policy::NoMemoryTagging,
                                kb::memory::policy::NoMemoryTracking>
    PoolArena;
#else
typedef kb::memory::MemoryArena<kb::memory::LinearAllocator, kb::memory::policy::SingleThread,
                                kb::memory::policy::SimpleBoundsChecking, kb::memory::policy::NoMemoryTagging,
                                kb::memory::policy::SimpleMemoryTracking>
    LinearArena;

typedef kb::memory::MemoryArena<kb::memory::PoolAllocator, kb::memory::policy::SingleThread,
                                kb::memory::policy::SimpleBoundsChecking, kb::memory::policy::NoMemoryTagging,
                                kb::memory::policy::SimpleMemoryTracking>
    PoolArena;
#endif

} // namespace erwin