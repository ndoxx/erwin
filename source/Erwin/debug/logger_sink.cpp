#include "debug/logger_sink.h"
#include "debug/logger_common.h"

#include <iostream>
#include <iomanip>
#include <regex>
#include <fstream>

namespace erwin
{
namespace dbg
{

void ConsoleSink::send(const LogStatement& stmt, const LogChannel& chan)
{
	if(stmt.msg_type != MsgType::RAW)
	{
		// Show file and line if sufficiently severe
		if(stmt.severity >= 2)
		{
			std::cout << "\033[1;38;2;255;255;255m@ " << stmt.code_file << ":" 
			          << "\033[1;38;2;255;90;90m" << stmt.code_line << "\n";
		}

	    float ts = std::chrono::duration_cast<std::chrono::duration<float>>(stmt.timestamp).count();
	    std::cout << "\033[1;38;2;0;130;10m["
	              << std::setprecision(6) << std::fixed
	              << ts << "]" << "\033[0m";
	    std::cout << chan.tag << " " << STYLES[stmt.msg_type] << ICON[stmt.msg_type] << stmt.message;
	}
	else
		std::cout << "\033[0m" << stmt.message << "\033[0m";
}

void ConsoleSink::send_raw(const std::string& message)
{
	std::cout << message;
}

LogFileSink::LogFileSink(const std::string& filename):
filename_(filename)
{

}

void LogFileSink::send(const LogStatement& stmt, const LogChannel& chan)
{
	if(stmt.msg_type != MsgType::RAW)
	{
		// Show file and line if sufficiently severe
		if(stmt.severity >= 2)
			ss_ << "@ " << stmt.code_file << ":" << stmt.code_line << std::endl;

    	float ts = std::chrono::duration_cast<std::chrono::duration<float>>(stmt.timestamp).count();
		ss_ << "[" << std::setprecision(6) << std::fixed << ts << "](" << int(stmt.severity) << ") ";
		ss_ << stmt.message;

		entries_.push_back(ss_.str());
	    ss_.str("");
	}
	else
		entries_.push_back(stmt.message);
}

void LogFileSink::send_raw(const std::string& message)
{
	entries_.push_back(message);
}

static std::string strip_ansi(const std::string& str)
{
	static std::regex ansi_regex("\033\\[.+?m"); // matches ANSI codes
	return std::regex_replace(str, ansi_regex, "");
}

void LogFileSink::finish()
{
	if(!enabled_)
		return;

	std::ofstream ofs(filename_);

	for(const std::string& entry: entries_)
		ofs << strip_ansi(entry);

	ofs.close();

	std::cout << "\033[1;38;2;255;255;255mSaved log file: " 
		      << "\033[1;38;2;90;255;90m" << filename_ << "\n";
}

} // namespace dbg
} // namespace erwin