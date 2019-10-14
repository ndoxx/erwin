#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "render_queue.hpp"

namespace erwin
{

struct StubQueueData
{
    typedef uint64_t RenderKey;
    
    int value;
};


class RenderThread
{
public:
    RenderThread();
    ~RenderThread();

    // Launch render thread
    void spawn();
    // Stop render thread execution (waits for pending commands to be processed)
    void kill();
    // Push a single item into the queue (inefficient)
    /*void enqueue(RenderKey key, QueueItem&& item);
    // Push a group of items into the queue
    void enqueue(const std::vector<RenderKey>& keys, std::vector<QueueItem>&& items);*/
    // Sort queue and dispatch items
    void flush();

protected:
    enum State: int
    {
        STATE_IDLE,
        STATE_FLUSH,
        STATE_KILLED
    };

    void thread_init();
    void thread_run();
    void thread_cleanup();

    //void dispatch(const QueueItem& item);

private:
    RenderQueue<StubQueueData> render_queue_;

    std::mutex mutex_;
    std::condition_variable cv_consume_;
    std::condition_variable cv_update_;
    std::thread render_thread_;
    std::atomic<int> thread_state_;
};

} // namespace erwin
