#pragma once

#include "memory/memory.hpp"
#include "memory/linear_allocator.h"

namespace erwin
{

#ifdef ARENA_RETAIL
typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::NoBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::NoMemoryTracking> LinearArena;
#else
typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> LinearArena;
#endif


} // namespace erwin