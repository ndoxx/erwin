#include <iostream>
#include <chrono>

#include "render_thread.h"

namespace erwin
{

RenderThread::RenderThread():
thread_state_(STATE_IDLE)
{

}

RenderThread::~RenderThread()
{
    if(render_thread_.joinable())
        kill();
}

void RenderThread::spawn()
{
    render_thread_ = std::thread(&RenderThread::thread_run, this);
}

void RenderThread::kill()
{
    // Terminate render thread execution and join
    thread_state_.store(STATE_KILLED, std::memory_order_release);
    // Must wake up thread before it can be killed
    cv_consume_.notify_one();
    render_thread_.join();
}
/*
void RenderThread::enqueue(RenderKey key, QueueItem&& item)
{
    // Avoid ackward deadlock on cv_update_ when thread is killed 
    // but another thread wants to push some draw items
    if(thread_state_.load(std::memory_order_acquire) == STATE_KILLED)
        return;

    // Wait for render thread to be idle
    std::unique_lock<std::mutex> lock(mutex_);
    cv_update_.wait(lock, [this]()
    {
        return thread_state_.load(std::memory_order_acquire) == STATE_IDLE;
    });
    render_queue_.push(key, std::forward<QueueItem>(item));
}

void RenderThread::enqueue(const std::vector<RenderKey>& keys,
                           std::vector<QueueItem>&& items)
{
    // Avoid ackward deadlock on cv_update_ when thread is killed 
    // but another thread wants to push some draw items
    if(thread_state_.load(std::memory_order_acquire) == STATE_KILLED)
        return;

    // Wait for render thread to be idle
    std::unique_lock<std::mutex> lock(mutex_);
    cv_update_.wait(lock, [this]()
    {
        return thread_state_.load(std::memory_order_acquire) == STATE_IDLE;
    });
    render_queue_.push(keys, std::forward<std::vector<QueueItem>>(items));
}
*/
void RenderThread::flush()
{
    // Force render thread to flush the queue
    thread_state_.store(STATE_FLUSH, std::memory_order_release);
    // Wake up render thread
    cv_consume_.notify_one();
}
/*
void RenderThread::dispatch(const QueueItem& item)
{
    std::cout << item.id << " ";
}*/

void RenderThread::thread_run()
{
    thread_init();

    while(true)
    {
        std::unique_lock<std::mutex> lck(mutex_);
        // Notify producer thread(s) the render thread is idle and
        // can accept items in its queue
        cv_update_.notify_all();
        // Wait for a state change
        cv_consume_.wait(lck, [this]()
        {
            return thread_state_.load(std::memory_order_acquire) != STATE_IDLE;
        });

        // Here: STATE_FLUSH or STATE_KILLED
        // Acquire state
        int state = thread_state_.load(std::memory_order_acquire);
        // Flush the queue and dispatch items
        /*render_queue_.flush(std::bind(&RenderThread::dispatch, this, std::placeholders::_1));
        std::cout << std::endl;*/

        // If state was STATE_FLUSH, go back to idle state
        if(state == STATE_KILLED)
            break;
        else
            thread_state_.store(STATE_IDLE, std::memory_order_release);
    }

    thread_cleanup();
}

void RenderThread::thread_init()
{
    std::cout << "Render Thread: init" << std::endl;
}

void RenderThread::thread_cleanup()
{
    std::cout << "Render Thread: cleanup" << std::endl;

}



} // namespace erwin
