#include "core/job_system.h"
#include "debug/logger.h"
#include "memory/arena.h"

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <algorithm>
#include "atomic_queue/atomic_queue.h"

/*
	TODO:
	[X] Use AtomicQueue instead of std::queue + mutex to reduce overhead
		-> https://github.com/max0x7ba/atomic_queue.git
	[ ] Handle job dependencies
		-> Maybe with a list of atomic counters like in:
			https://archive.org/details/GDC2015Gyrling_201508

	[ ] Work stealing scheme?
		-> Probably not.
*/

namespace erwin
{

// Job queue configuration
static constexpr size_t k_max_jobs = 128;
static constexpr bool   k_minimize_contention = true;
static constexpr bool   k_maximize_throughput = true;
static constexpr bool   k_SPSC = false;

// Handles
constexpr size_t k_handle_alloc_size = 2 * sizeof(RobustHandlePoolT<k_max_jobs>);
ROBUST_HANDLE_DEFINITION( JobHandle, k_max_jobs );


struct Job
{
	JobSystem::JobFunction function = [](){};
	JobHandle handle;
	JobHandle parent;
	bool alive = true;
};
/*
struct AtomicWrapper
{
	AtomicWrapper(const AtomicWrapper& other): 
	counter(other.counter.load())
	{}

	AtomicWrapper& operator=(const AtomicWrapper& other)
	{
		counter.store(other.counter.load());
		return *this;
	}

	std::atomic<uint32_t> counter;
	// + padding to avoid false sharing?
};
*/
class WorkerThread
{
public:
	WorkerThread(uint32_t tid): tid_(tid), thread_(&WorkerThread::run, this) { }

	void run();
	inline void join() { thread_.join(); }

private:
	uint32_t tid_;
	std::thread thread_;
	std::vector<Job> wait_list_; // To store jobs that have unmet dependencies
};

static struct JobSystemStorage
{
	template <typename T>
	using AtomicQueue = atomic_queue::AtomicQueue<T,
												  k_max_jobs, 
												  T { },
												  k_minimize_contention, 
												  k_maximize_throughput, 
												  false, // TOTAL_ORDER
												  k_SPSC>;

	std::vector<WorkerThread> threads;
	std::vector<Job*> alive_jobs;
	AtomicQueue<Job*> jobs;
    std::mutex wake_mutex;
    std::mutex wait_mutex;
    std::condition_variable cv_wake; // To wake worker threads
    std::condition_variable cv_wait; // For the main thread to wait on
    std::atomic<bool> running;
    std::atomic<uint64_t> status;
    uint64_t current_status;
    uint32_t threads_count;

	LinearArena handle_arena;
	PoolArena job_pool;
} s_storage;


void WorkerThread::run()
{
    while(s_storage.running.load(std::memory_order_acquire))
	{
        // Get a job you lazy bastard
        if(!s_storage.jobs.was_empty())
        {
    		Job* job = s_storage.jobs.pop();

    		// Check dependencies
    		/*if(job->parent.is_valid())
    		{
    			// Parent job has not finished yet, push the job into the wait list
    			wait_list_.push_back(job);
    			continue;
    		}*/

        	// DLOG("thread",1) << "Thread " << tid_ << " took the job " << job->id << std::endl;
    		job->function();
    		job->handle.release();
    		job->alive = false;
    		// Notify main thread that the job is done
        	s_storage.status.fetch_add(1, std::memory_order_relaxed);
        	s_storage.cv_wait.notify_one();
        	// DLOG("thread",1) << "Thread " << tid_ << " finished the job " << job->id << std::endl;
        }
        else // No job -> wait
        {
        	std::unique_lock<std::mutex> lck(s_storage.wake_mutex);
        	s_storage.cv_wake.wait(lck); // Spurious wakeups have no effect because we check if the queue is empty
        }
	}
}


void JobSystem::init(memory::HeapArea& area)
{
	DLOGN("thread") << "Initializing job system." << std::endl;

	// Initialize memory
	size_t max_align = 64-1;
	size_t node_size = sizeof(Job) + max_align;
	s_storage.handle_arena.init(area.require_block(k_handle_alloc_size));
	s_storage.job_pool.init(area.require_pool_block<PoolArena>(node_size, k_max_jobs), node_size, k_max_jobs, PoolArena::DECORATION_SIZE);
	JobHandle::init_pool(s_storage.handle_arena);

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
		s_storage.threads.emplace_back(ii);

	DLOGI << "done" << std::endl;
}

void JobSystem::shutdown()
{
	DLOGN("thread") << "Waiting for jobs to finish." << std::endl;
	wait();
	DLOGN("thread") << "Shutting down job system." << std::endl;

	// Notify all threads they are going to die
    s_storage.running.store(false, std::memory_order_release);
    s_storage.cv_wake.notify_all();
	for(uint32_t ii=0; ii<s_storage.threads_count; ++ii)
    	s_storage.threads[ii].join();

	cleanup();

	JobHandle::destroy_pool(s_storage.handle_arena);

	DLOGI << "done" << std::endl;
}

void JobSystem::cleanup()
{
	// Return dead jobs to the pool
	std::remove_if(s_storage.alive_jobs.begin(), s_storage.alive_jobs.end(), [](Job* job)
	{
		if(!job->alive)
		{
			W_DELETE(job, s_storage.job_pool);
			return true;
		}
		return false;
	});
}

JobHandle JobSystem::schedule(JobFunction function)
{
	cleanup();

	JobHandle handle = JobHandle::acquire();
	++s_storage.current_status;

	// Enqueue new job
	// DLOG("thread",1) << "Enqueuing job " << handle.index << std::endl;
	Job* job = W_NEW_ALIGN(Job, s_storage.job_pool, 64){ function, handle };
    s_storage.jobs.push(job);
    s_storage.alive_jobs.push_back(job);

    // Wake one worker thread
	s_storage.cv_wake.notify_one();

	return handle;
}

bool JobSystem::is_busy()
{
	return s_storage.status.load(std::memory_order_relaxed) < s_storage.current_status;
}

bool JobSystem::is_work_done(JobHandle handle)
{
	return !handle.is_valid();
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
        return !is_busy();
    });
}

void JobSystem::wait_for(JobHandle handle)
{
	// Same as before but we wait for the passed handle to be invalidated by a worker thread.
    std::unique_lock<std::mutex> lock(s_storage.wait_mutex);
    s_storage.cv_wait.wait(lock, [&handle]()
    {
        return !handle.is_valid();
    });
}


} // namespace erwin