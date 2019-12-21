#include <thread>
#include <chrono>

#include "catch2/catch.hpp"
#include "memory/memory.hpp"
#include "core/job_system.h"
#include "debug/logger.h"

using namespace erwin;
using namespace std::chrono_literals;

class JobFixture
{
public:
	JobFixture():
	area(32_kB)
	{
		JobSystem::init(area);
	}

	~JobFixture()
	{
		JobSystem::shutdown();
	}

protected:
	memory::HeapArea area;
};

TEST_CASE_METHOD(JobFixture, "Spawning a job", "[job]")
{
	JobHandle handle = JobSystem::schedule([]()
	{
		std::this_thread::sleep_for(50ms);
	});

	// Job is launched asynchronously and should not be finished now
	REQUIRE(!JobSystem::is_work_done(handle));

	// Wait for job to finish
	JobSystem::wait_for(handle);
	REQUIRE(JobSystem::is_work_done(handle));
}