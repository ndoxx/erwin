#pragma once

#include "logger_common.h"

namespace erwin
{
namespace dbg
{

// Output stream that will synchronize with the logger thread queue on std::endl
class LoggerStream: public std::ostream
{
private:
	// Custom stringbuf to react to std::endl
	class StringBuffer: public std::stringbuf
	{
	public:
	    explicit StringBuffer(LoggerStream& parent);
	    ~StringBuffer();

	    // This override allows to monitor std::endl
		virtual int sync() override;
		// Let parent submit the completed message with some state, and clear buffer
		inline void submit() {	parent_.submit(str()); str(""); }

	private:
		LoggerStream& parent_;
	};

public:
	LoggerStream();
	~LoggerStream() = default;

	// Initialize log message state attributes
	void prepare(hash_t channel, MsgType msg_type, uint8_t severity, int code_line, const char* code_file);

private:
	// Send message and current state to logger thread
	void submit(const std::string& message);

	StringBuffer buffer_; 			// The buffer this stream writes to
	LogStatement stmt_;		    	// Current state
};

// Return a unique stream per invoker thread, using thread local storage
static inline LoggerStream& get_log(hash_t channel, MsgType msg_type, uint8_t severity, int code_line=0, const char* code_file="")
{
    thread_local LoggerStream ls;
	ls.prepare(channel, msg_type, severity, code_line, code_file);
    return ls;
}

} // namespace dbg

// These macros will be optimized out (and arguments not evaluated)
// when LOGGING_ENABLED is set to 0. Should be better performance wise than using a null stream.
#define DLOG(C,S) if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::NORMAL, (S) )
#define DLOGI     if constexpr(!LOGGING_ENABLED); else get_log(                 0, erwin::dbg::MsgType::ITEM, 4)
#define DLOGR(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::RAW, 0)
#define DLOGN(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::NOTIFY, 0)
#define DLOGW(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::WARNING, 1, __LINE__, __FILE__ )
#define DLOGE(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::ERROR, 2, __LINE__, __FILE__ )
#define DLOGF(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::FATAL, 3, __LINE__, __FILE__ )
#define DLOGG(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::GOOD, 3, __LINE__, __FILE__ )
#define DLOGB(C)  if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::BAD, 3, __LINE__, __FILE__ )
#define BANG()    if constexpr(!LOGGING_ENABLED); else get_log( erwin::H_("core"), erwin::dbg::MsgType::BANG, 3) << __FILE__ << ":" << __LINE__ << std::endl

#define DLOGR__(C) get_log( erwin::H_( (C) ) , erwin::dbg::MsgType::RAW, 0)


} // namespace erwin
