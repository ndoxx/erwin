#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>

#include "debug/logger_thread.h"
#include "debug/logger.h"
#include "debug/stack_trace.h"
#include "math/color.h"

namespace erwin
{
namespace dbg
{

static const uint32_t CHANNEL_STYLE_PALETTE = 16u;

static std::map<MsgType, WCC> STYLES =
{
    {MsgType::NORMAL,    WCC(255,255,255)},
    {MsgType::ITEM,      WCC(255,255,255)},
    {MsgType::EVENT,     WCC(255,255,255)},
    {MsgType::NOTIFY,    WCC(150,130,255)},
    {MsgType::WARNING,   WCC(255,175,0)},
    {MsgType::ERROR,     WCC(255,90, 90)},
    {MsgType::FATAL,     WCC(255,0,  0)},
    {MsgType::BANG,      WCC(255,100,0)},
    {MsgType::GOOD,      WCC(0,  255,0)},
    {MsgType::BAD,       WCC(255,0,  0)},
};

static std::map<MsgType, std::string> ICON =
{
    {MsgType::NORMAL,    "    "},
    {MsgType::ITEM,      "     \u21B3 "},
    {MsgType::EVENT,     " \u2107 "},
    {MsgType::NOTIFY,    "\033[1;48;2;20;10;50m \u2055 \033[1;49m "},
    {MsgType::WARNING,   "\033[1;48;2;50;40;10m \u203C \033[1;49m "},
    {MsgType::ERROR,     "\033[1;48;2;50;10;10m \u2020 \033[1;49m "},
    {MsgType::FATAL,     "\033[1;48;2;50;10;10m \u2021 \033[1;49m "},
    {MsgType::BANG,      "\033[1;48;2;50;40;10m \u0489 \033[1;49m "},
    {MsgType::GOOD,      "\033[1;48;2;10;50;10m \u203F \033[1;49m "},
    {MsgType::BAD,       "\033[1;48;2;50;10;10m \u2054 \033[1;49m "},
};

Sink::Sink():
enabled_(true)
{

}

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

LogFileSink::LogFileSink(const char* filename):
filename_(filename)
{

}

LogFileSink::~LogFileSink()
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


LoggerThread::LoggerThread():
thread_state_(STATE_IDLE),
backtrace_on_error_(false),
single_threaded_(false)
{
	create_channel("core", 3);
	create_channel("event", 3);
}

LoggerThread::~LoggerThread()
{
    if(logger_thread_.joinable())
        kill();

	for(auto&& [key,sink]: sinks_)
		sink->finish();
}

void LoggerThread::create_channel(const std::string& name, uint8_t verbosity)
{
	hash_t hname = H_(name.c_str());

    // Detect duplicate channel or hash collision
    auto it = channels_.find(hname);
    if(it != channels_.end())
    {
    	std::cout << "Duplicate channel (or hash collision) -> ignoring channel \'" << it->second.name << "\'" << std::endl;
        return;
    }

	std::string short_name = name.substr(0,3);
	glm::vec3 bgcolor = color::random_color(hname+CHANNEL_STYLE_PALETTE, 0.8f, 0.4f);
	
	std::stringstream ss;
	ss << "\033[1;48;2;" << uint32_t(bgcolor[0]*255) << ";" << uint32_t(bgcolor[1]*255) << ";" << uint32_t(bgcolor[2]*255) << "m";
	ss << "[" << short_name << "]" << "\033[0m";
	std::string tag = ss.str();

	channels_.insert(std::make_pair(H_(name.c_str()), LogChannel{verbosity, name, tag}));
}

void LoggerThread::attach(const std::string& sink_name, std::unique_ptr<Sink> sink, const std::vector<hash_t> channels)
{
	hash_t hsink = H_(sink_name.c_str());
	sinks_.insert(std::make_pair(hsink, std::move(sink)));

	for(hash_t channel: channels)
	{
		auto it = channels_.find(channel);
		if(it != channels_.end())
			sink_subscriptions_.insert(std::make_pair(channel, hsink));
	}
}

void LoggerThread::attach_all(const std::string& sink_name, std::unique_ptr<Sink> sink)
{
	hash_t hsink = H_(sink_name.c_str());
	sinks_.insert(std::make_pair(hsink, std::move(sink)));

	for(auto&& [key,chan]: channels_)
		sink_subscriptions_.insert(std::make_pair(key, hsink));
}

void LoggerThread::spawn()
{
	if(!single_threaded_)
    	logger_thread_ = std::thread(&LoggerThread::thread_run, this);
}

void LoggerThread::sync()
{
	if(single_threaded_) return;

    std::unique_lock<std::mutex> lock(mutex_);
    cv_update_.wait(lock, [this]()
    {
        return thread_state_.load(std::memory_order_acquire) == STATE_IDLE;
    });
}

void LoggerThread::kill()
{
	if(single_threaded_) return;

    // Terminate logger thread execution and join
    thread_state_.store(STATE_KILLED, std::memory_order_release);
    // Must wake up thread before it can be killed
    cv_consume_.notify_one();
    logger_thread_.join();
}

void LoggerThread::enqueue(LogStatement&& stmt)
{
	if(single_threaded_)
	{
    	std::unique_lock<std::mutex> lock(mutex_);
		dispatch(stmt);
		return;
	}

	// Avoid ackward deadlock on cv_update when thread is killed 
	// but another thread wants to push some log data
	if(thread_state_.load(std::memory_order_acquire) == STATE_KILLED)
		return;

    // Wait for logger thread to be idle
    std::unique_lock<std::mutex> lock(mutex_);
    cv_update_.wait(lock, [this]()
    {
        return thread_state_.load(std::memory_order_acquire) == STATE_IDLE;
    });
    log_statements_.push_back(std::forward<LogStatement>(stmt));
}

void LoggerThread::enqueue(const LogStatement& stmt)
{
	if(single_threaded_)
	{
    	std::unique_lock<std::mutex> lock(mutex_);
		dispatch(stmt);
		return;
	}

	// Avoid ackward deadlock on cv_update when thread is killed 
	// but another thread wants to push some log data
	if(thread_state_.load(std::memory_order_acquire) == STATE_KILLED)
		return;

    // Wait for logger thread to be idle
    std::unique_lock<std::mutex> lock(mutex_);
    cv_update_.wait(lock, [this]()
    {
        return thread_state_.load(std::memory_order_acquire) == STATE_IDLE;
    });
    log_statements_.push_back(stmt);
}

void LoggerThread::flush()
{
	if(single_threaded_) return;

    // Force logger thread to flush the queue
    thread_state_.store(STATE_FLUSH, std::memory_order_release);
    // Wake up logger thread
    cv_consume_.notify_one();
}

void LoggerThread::set_sink_enabled(hash_t name, bool value)
{
    std::unique_lock<std::mutex> lock(mutex_);
	sinks_.at(name)->set_enabled(value);
}

void LoggerThread::dispatch(const LogStatement& stmt)
{
	if(stmt.msg_type == MsgType::BANG)
	{
		std::cout << "  " << STYLES[MsgType::BANG] << ICON[MsgType::BANG] << stmt.message;
		return;
	}

	auto it = channels_.find(stmt.channel);
	if(it==channels_.end())
	{
		std::cout << "Channel " << stmt.channel << " does not exist." << std::endl;
		return;
	}

	auto& chan = it->second;

	// check out all sinks subscribed to current channel
	auto&& range = sink_subscriptions_.equal_range(stmt.channel);

	uint8_t required_verbosity = 3 - ((stmt.severity>3) ? 3 : stmt.severity);
	if(chan.verbosity >= required_verbosity)
	{
		for(auto&& it=range.first; it!=range.second; ++it)
		{
			if(sinks_.at(it->second)->is_enabled())
			{
				auto&& sink = sinks_.at(it->second);
				sink->send(stmt, chan);

				// send backtrace if required
				if(stmt.severity > 1 && backtrace_on_error_)
				{
					std::string backtrace_log(get_backtrace());
					sink->send_raw("\033[1;38;2;255;100;0m-------/ \033[1;38;2;255;200;0mBACKTRACE\033[1;38;2;255;100;0m \\-------\n");
					sink->send_raw("\033[1;38;2;220;220;220m" + backtrace_log + "\033[1;38;2;255;100;0m---------------------------\n");
				}
			}
		}
	}
}

void LoggerThread::thread_run()
{
    thread_init();

    while(true)
    {
        std::unique_lock<std::mutex> lck(mutex_);
        // Notify producer thread(s) the logger thread is idle and
        // can accept statements in its queue
        cv_update_.notify_all();
        // Wait for a state change
        cv_consume_.wait(lck, [this]()
        {
            return thread_state_.load(std::memory_order_acquire) != STATE_IDLE;
        });

        // Here: STATE_FLUSH or STATE_KILLED
        // Acquire state
        int state = thread_state_.load(std::memory_order_acquire);
        // Sort the queue according to timestamps
        // Could be optimized by sorting indices instead, but no real performance gain
        std::sort(log_statements_.begin(), log_statements_.end(), [](const auto& lhs, const auto& rhs)
		{
		   return lhs.timestamp < rhs.timestamp;
		});
        // Flush the queue and dispatch statements
        for(auto&& stmt: log_statements_)
        	dispatch(stmt);
        log_statements_.clear();

        // If state was STATE_FLUSH, go back to idle state
        if(state == STATE_KILLED)
            break;
        else
            thread_state_.store(STATE_IDLE, std::memory_order_release);
    }

    thread_cleanup();
}

void LoggerThread::thread_init()
{
    std::cout << "\033[0mLogger Thread: init" << std::endl;
}

void LoggerThread::thread_cleanup()
{
    std::cout << "\033[0mLogger Thread: cleanup" << std::endl;
}

#ifdef LOGGING_ENABLED
std::unique_ptr<LoggerThread> Logger::LOGGER_THREAD = std::make_unique<LoggerThread>();
#endif

} // namespace dbg
} // namespace erwin