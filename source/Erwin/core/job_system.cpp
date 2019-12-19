#include "core/job_system.h"
#include "debug/logger.h"
#include "memory/arena.h"

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

namespace erwin
{

HANDLE_DEFINITION( JobHandle );

struct Job
{
	JobSystem::JobFunction function;
};

static struct JobSystemStorage
{
	std::vector<std::thread> threads;
	std::queue<Job*> jobs;
	std::mutex job_queue_mutex;
    std::mutex wake_mutex;
    std::condition_variable cv_wake;
    std::atomic<bool> running;
    std::atomic<uint32_t> num_tasks;

	PoolArena* p_job_pool;
    uint32_t threads_count;
} s_storage;

static void thread_run(uint32_t tid)
{
    while(s_storage.running.load(std::memory_order_acquire))
	{
        // Get a job you lazy bastard
        if(!s_storage.jobs.empty())
        {
        	Job* job = nullptr;
	        {
	        	std::lock_guard<std::mutex> job_lock(s_storage.job_queue_mutex);
	        	job = s_storage.jobs.front();
	        	s_storage.jobs.pop();
	        }
	        if(job)
	        {
	        	s_storage.num_tasks.fetch_add(1, std::memory_order_release);
        		job->function();
	        	s_storage.num_tasks.fetch_sub(1, std::memory_order_release);
        		W_DELETE(job, (*s_storage.p_job_pool));
        	}
        }
        else // No job -> wait
        {
        	std::unique_lock<std::mutex> lck(s_storage.wake_mutex);
        	s_storage.cv_wake.wait(lck);
        }
	}
}

void JobSystem::init(memory::HeapArea& area, size_t max_jobs)
{
	DLOGN("thread") << "Initializing job system." << std::endl;

	// Initialize memory
    size_t max_align = 64-1;
	size_t node_size = sizeof(Job) + max_align;
	s_storage.p_job_pool = new PoolArena(area.require_pool_block<PoolArena>(node_size, max_jobs), node_size, max_jobs, PoolArena::DECORATION_SIZE);

	// Find the number of CPU cores
	uint32_t CPU_cores_count = std::thread::hardware_concurrency();
	s_storage.threads_count = CPU_cores_count - 1;

	DLOGI << "Detected " << WCC('v') << CPU_cores_count << WCC(0) << " CPU cores." << std::endl;
	DLOGI << "Spawning " << WCC('v') << s_storage.threads_count << WCC(0) << " worker threads." << std::endl;

	// Spawn N-1 threads
    s_storage.running.store(true, std::memory_order_release);
    s_storage.num_tasks.store(0, std::memory_order_release);
	for(uint32_t ii=0; ii<s_storage.threads_count; ++ii)
		s_storage.threads.emplace_back(&thread_run, ii);

	DLOGI << "done" << std::endl;
}

void JobSystem::shutdown()
{
	DLOGN("thread") << "Shutting down job system." << std::endl;

    s_storage.running.store(false, std::memory_order_release);
    s_storage.cv_wake.notify_all();
	for(uint32_t ii=0; ii<s_storage.threads_count; ++ii)
    	s_storage.threads[ii].join();

	delete s_storage.p_job_pool;

	DLOGI << "done" << std::endl;
}

void JobSystem::execute(JobFunction function)
{
	// Enqueue new job
	Job* job = W_NEW_ALIGN(Job, (*s_storage.p_job_pool), 64) { function };
    {
    	std::lock_guard<std::mutex> job_lock(s_storage.job_queue_mutex);
    	s_storage.jobs.push(job);
    }

    // Wake one worker thread
	s_storage.cv_wake.notify_one();
}

void JobSystem::wait()
{
	while(s_storage.num_tasks.load(std::memory_order_acquire))
	{
		s_storage.cv_wake.notify_one();
		std::this_thread::yield();
	}
}


} // namespace erwin