#pragma once

#include "memory/memory.hpp"
#include "memory/linear_allocator.h"
#include "memory/pool_allocator.h"

namespace erwin
{

#ifdef ARENA_RETAIL
typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::NoBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::NoMemoryTracking> LinearArena;

typedef memory::MemoryArena<memory::PoolAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::NoBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::NoMemoryTracking> PoolArena;
#else
typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> LinearArena;

typedef memory::MemoryArena<memory::PoolAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> PoolArena;
#endif


} // namespace erwin