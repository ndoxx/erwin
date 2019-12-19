#pragma once

#include <functional>
#include <atomic>

#include "core/handle.h"
#include "memory/memory.hpp"

namespace erwin
{

HANDLE_DECLARATION( JobHandle );

class JobSystem
{
public:

	using JobFunction = std::function<void(void)>;

	static void init(memory::HeapArea& area, size_t max_jobs);
	static void shutdown();
	static void execute(JobFunction function);
	static void wait();
};


} // namespace erwin