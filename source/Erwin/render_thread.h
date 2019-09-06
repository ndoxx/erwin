#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "render_queue.h"

namespace erwin
{

class RenderThread
{
public:
    RenderThread();
    ~RenderThread();

    // Launch render thread
    void spawn();
    // Stop render thread execution (waits for pending commands to be processed)
    void kill();
    // Push a single command into the queue (inefficient)
    void enqueue(RenderKey key, RenderCommand&& command);
    // Push a group of commands into the queue
    void enqueue(const std::vector<RenderKey>& keys, std::vector<RenderCommand>&& commands);
    // Sort queue and dispatch commands
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

    void dispatch(const RenderCommand& command);

private:
    RenderQueue render_queue_;

    std::mutex mutex_;
    std::condition_variable cv_consume_;
    std::condition_variable cv_update_;
    std::thread render_thread_;
    std::atomic<int> thread_state_;
};

} // namespace erwin
