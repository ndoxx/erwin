#pragma once

#include <functional>
#include <atomic>

#include "core/handle.h"
#include "memory/memory.hpp"

namespace erwin
{

ROBUST_HANDLE_DECLARATION( JobHandle );

class JobSystem
{
public:
	using JobFunction = std::function<void(void)>;

	// Initialize job system's memory and spawn worker threads
	static void init(memory::HeapArea& area);
	// Wait for all jobs to finish, join worker threads and destroy system storage
	static void shutdown();
	// Enqueue a new job for asynchronous execution and return a handle for this job
	// This should not be used for jobs with dependencies
	static JobHandle schedule(JobFunction function);
	// Non-blockingly check if any worker threads are busy
	static bool is_busy();
	// Non-blockingly check if a job is processed
	static bool is_work_done(JobHandle handle);
	// Hold execution on this thread till all jobs are processed
	static void wait();
	// Hold execution on this thread till a particular job is processed
	static void wait_for(JobHandle handle);

	static void cleanup();
};


} // namespace erwin