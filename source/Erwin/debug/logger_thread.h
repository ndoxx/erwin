#pragma once

#include <ostream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "ctti/type_id.hpp"

#include "logger_common.h"
#include "../event/event.h"

namespace erwin
{
namespace dbg
{

// Base class for logger sinks.
// A sink can be registered by the logger thread and will be fed
// the log statements that have been queued up each time the queue is flushed.
class Sink
{
public:
	Sink();
	virtual ~Sink() = default;

	// Submit a log statement to this sink, specifying the channel it emanates from
	virtual void send(const LogStatement& stmt, const LogChannel& chan) = 0;
	// Submit a raw string to this sink
	virtual void send_raw(const std::string& message) = 0;
	// Override this if some operations need to be performed before logger destruction
	virtual void finish() {}

	inline void set_enabled(bool value) { enabled_ = value; }
	inline bool is_enabled() const      { return enabled_; }

protected:
	bool enabled_;
};

// This sink writes to the terminal with ANSI color support
class ConsoleSink: public Sink
{
public:
	virtual ~ConsoleSink() = default;
	virtual void send(const LogStatement& stmt, const LogChannel& chan) override;
	virtual void send_raw(const std::string& message) override;
};

// This sink writes to a file (ANSI codes are stripped away)
class LogFileSink: public Sink
{
public:
	LogFileSink(const char* filename);
	virtual ~LogFileSink();
	virtual void send(const LogStatement& stmt, const LogChannel& chan) override;
	virtual void send_raw(const std::string& message) override;
	virtual void finish() override;

private:
	const char* filename_;
	std::stringstream ss_;
	std::vector<std::string> entries_;
};

class LoggerThread
{
public:
    LoggerThread();
    ~LoggerThread();

    // Create a logging channel to group information of the same kind
    void create_channel(const std::string& name, uint8_t verbosity=3);
    // Attach a sink to a list of channels
	void attach(const std::string& sink_name, std::unique_ptr<Sink> sink, const std::vector<hash_t> channels);
    // Attach a sink to all channels
	void attach_all(const std::string& sink_name, std::unique_ptr<Sink> sink);
	// Forward events as text as they arrive
	template <typename EventT>
	void track_event()
	{
		EVENTBUS.subscribe(this, &LoggerThread::log_event<EventT>);
		event_filter_[ctti::type_id<EventT>()] = true;
	}
	// Enable/Disable tracking on a given registered event type
	template <typename EventT>
	void set_event_tracking_enabled(bool value)
	{
		event_filter_[ctti::type_id<EventT>()] = value;
	}

    // Launch logger thread
    void spawn();
    // Stop thread execution (waits for pending statements to be processed)
    void kill();
    // Push a single log statement into the queue
    void enqueue(LogStatement&& stmt);
	void enqueue(const LogStatement& stmt);
    // Ddispatch log statements to registered sinks
    void flush();

    // Get channel verbosity by name
    inline uint32_t get_channel_verbosity(hash_t name) const
    {
        return channels_.at(name).verbosity;
    }
    // Change channel verbosity
    inline void set_channel_verbosity(hash_t name, uint32_t verbosity)
    {
    	std::unique_lock<std::mutex> lock(mutex_);
        channels_.at(name).verbosity = std::min(verbosity, 3u);
    }
    // Mute channel by setting its verbosity to 0
    inline void mute_channel(hash_t name)
    {
    	std::unique_lock<std::mutex> lock(mutex_);
        channels_.at(name).verbosity = 0;
    }
    // Enable/Disable backtrace printing on error message
    inline void set_backtrace_on_error(bool value)
    {
    	std::unique_lock<std::mutex> lock(mutex_);
        backtrace_on_error_ = value;
    }
	// Enable/Disable a registered tracker
	void set_sink_enabled(hash_t name, bool value);

protected:
    enum State: int
    {
        STATE_IDLE,	 // The thread does nothing and is ready to queue more statements
        STATE_FLUSH, // The queue is emptied and the statements dispatched to sinks
        STATE_KILLED // The thread must halt properly and will join
    };

    // Called before the thread enters its main loop
    void thread_init();
    // Serve queuing requests (state machine impl)
    void thread_run();
    // Called before the thread joins
    void thread_cleanup();

    // Send a log statement to each sink (depending on their channel subscriptions)
    void dispatch(const LogStatement& stmt);

    // Helper func to queue event data
    // TODO: MOVE to proper tracker system
	template <typename EventT>
	bool log_event(const EventT& event)
	{
		if(event_filter_[ctti::type_id<EventT>()])
		{
			std::stringstream ss;
			ss << "\033[1;38;2;0;0;0m\033[1;48;2;0;185;153m[" << event.get_name() << "]\033[0m " << event << std::endl;
			enqueue(LogStatement{"event"_h, dbg::MsgType::EVENT, event.timestamp, 0, 0, "", ss.str()});
		}
        return false;
	}

private:
    std::vector<LogStatement> log_statements_; // Stores log statements, access is synced
    std::mutex mutex_;						   // To lock/unlock access to the queue

    std::condition_variable cv_consume_; // Allows to wait passively for a state change
    std::condition_variable cv_update_;  // Allows to notify producer threads that the logger thread is ready to accept new statements
    std::thread logger_thread_;			 // Thread handle
    std::atomic<int> thread_state_;		 // Contains the state of this thread's state machine

	bool backtrace_on_error_; // Enable/Disable automatic backtrace submission on severe statements

	std::map<hash_t, std::unique_ptr<Sink>> sinks_;		     // Registered sinks by hash name
	std::map<hash_t, LogChannel> channels_;				     // Registered channels by hash name
	std::multimap<hash_t, size_t> sink_subscriptions_;	     // Sinks can subscribe to multiple channels
    std::unordered_map<ctti::unnamed_type_id_t, bool> event_filter_; // Controls which tracked events are propagated to the sinks
};

#ifdef LOGGING_ENABLED
struct Logger
{
	static std::unique_ptr<LoggerThread> LOGGER_THREAD;
};
#endif

} // namespace dbg

#define WLOGGER if constexpr(!LOGGING_ENABLED); else (*dbg::Logger::LOGGER_THREAD)

} // namespace erwin