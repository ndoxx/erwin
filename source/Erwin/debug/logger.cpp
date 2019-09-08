#include <iostream>
#include <iomanip>

#include "logger.h"
#include "logger_thread.h"

namespace erwin
{
namespace dbg
{

LoggerStream::StringBuffer::StringBuffer(LoggerStream& parent):
parent_(parent)
{

}

LoggerStream::StringBuffer::~StringBuffer()
{
	if (pbase() != pptr())
		submit();
}

int LoggerStream::StringBuffer::sync()
{
	submit();
	return 0;
}

LoggerStream::LoggerStream():
std::ostream(&buffer_),
buffer_(*this)
{
	stmt_ = {"core"_h, MsgType::NORMAL, TimeStamp(), 0, 0, ""};
}

void LoggerStream::prepare(hash_t channel, MsgType msg_type, uint8_t severity, int code_line, const char* code_file)
{
	if(channel == 0)
		channel = stmt_.channel;
	if(severity == 4)
		severity = stmt_.severity;

	stmt_.timestamp = TimeBase::timestamp();
	stmt_.channel = channel;
	stmt_.msg_type = msg_type;
	stmt_.severity = severity;
	stmt_.code_line = code_line;
	stmt_.code_file = code_file;
}

void LoggerStream::submit(const std::string& message)
{
	stmt_.message = message;
#ifdef LOGGING_ENABLED
    Logger::LOGGER_THREAD->enqueue(stmt_);
#endif
}

} // namespace dbg
} // namespace erwin
	