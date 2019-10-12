#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace erwin
{
namespace dbg
{

struct LogStatement;
struct LogChannel;

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
	LogFileSink(const std::string& filename);
	virtual ~LogFileSink() = default;
	virtual void send(const LogStatement& stmt, const LogChannel& chan) override;
	virtual void send_raw(const std::string& message) override;
	virtual void finish() override;

private:
	std::string filename_;
	std::stringstream ss_;
	std::vector<std::string> entries_;
};

} // namespace dbg
} // namespace erwin