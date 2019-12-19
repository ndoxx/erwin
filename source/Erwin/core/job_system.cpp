#include "core/job_system.h"
#include "debug/logger.h"
#include "memory/arena.h"

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

/*
	TODO:
	[ ] Use AtomicQueue instead of std::queue + mutex to reduce overhead
		-> https://github.com/max0x7ba/atomic_queue
	[ ] Handle job dependencies
	[ ] Work stealing scheme?
*/

namespace erwin
{

HANDLE_DEFINITION( JobHandle );

struct Job
{
	JobSystem::JobFunction function;
	uint64_t id;
};

static struct JobSystemStorage
{
	std::vector<std::thread> threads;
	std::queue<Job*> jobs;
	std::mutex job_queue_mutex;
    std::mutex wake_mutex;
    std::mutex wait_mutex;
    std::condition_variable cv_wake; // To wake worker threads
    std::condition_variable cv_wait; // For the main thread to wait on
    std::atomic<bool> running;
    std::atomic<uint64_t> status;
    uint64_t current_status;
    uint32_t threads_count;

	PoolArena* p_job_pool;
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
	        	DLOG("thread",1) << "Thread " << tid << " took the job " << job->id << std::endl;
	        	s_storage.status.fetch_add(1, std::memory_order_relaxed);
        		job->function();
        		// Notify main thread that the job is done
	        	s_storage.cv_wait.notify_one();
        		W_DELETE(job, (*s_storage.p_job_pool));
	        	DLOG("thread",1) << "Thread " << tid << " finished the job " << job->id << std::endl;
        	}
        }
        else // No job -> wait
        {
        	std::unique_lock<std::mutex> lck(s_storage.wake_mutex);
        	s_storage.cv_wake.wait(lck); // Spurious wakeups have no effect because we check if the queue is empty
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
    s_storage.status.store(0, std::memory_order_release);
    s_storage.current_status = 0;
	for(uint32_t ii=0; ii<s_storage.threads_count; ++ii)
		s_storage.threads.emplace_back(&thread_run, ii);

	DLOGI << "done" << std::endl;
}

void JobSystem::shutdown()
{
	DLOGN("thread") << "Shutting down job system." << std::endl;

	// Notify all threads they are going to die
    s_storage.running.store(false, std::memory_order_release);
    s_storage.cv_wake.notify_all();
	for(uint32_t ii=0; ii<s_storage.threads_count; ++ii)
    	s_storage.threads[ii].join();

	delete s_storage.p_job_pool;

	DLOGI << "done" << std::endl;
}

void JobSystem::execute(JobFunction function)
{
	static uint64_t s_job_id = 0;
	++s_storage.current_status;

	// Enqueue new job
	Job* job = W_NEW_ALIGN(Job, (*s_storage.p_job_pool), 64) { function, s_job_id++ };
	DLOG("thread",1) << "Enqueuing job " << job->id << std::endl;
    {
    	std::lock_guard<std::mutex> job_lock(s_storage.job_queue_mutex);
    	s_storage.jobs.push(job);
    }

    // Wake one worker thread
	s_storage.cv_wake.notify_one();
}

void JobSystem::wait()
{
	// Main thread increments current_status each time a job is pushed to the queue.
	// Worker threads atomically increment status each time they finished a job.
	// Then we just need to wait for status to catch up with current_status in order
	// to be sure all worker threads have finished.
    std::unique_lock<std::mutex> lock(s_storage.wait_mutex);
    s_storage.cv_wait.wait(lock, []()
    {
        return s_storage.status.load(std::memory_order_relaxed) >= s_storage.current_status;
    });
}


} // namespace erwin