#include "application.h"
#include "clock.hpp"
#include "../debug/logger.h"
#include "../debug/logger_thread.h"

namespace erwin
{


Application::Application():
is_running_(true)
{
	
}

Application::~Application()
{
	
}

void Application::run()
{
	// Initialize logger
    WLOGGER.create_channel("application", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach_all("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"));
    // WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), {"event"_h});
    WLOGGER.set_backtrace_on_error(true);
    WLOGGER.spawn();

    DLOG("application",1) << "Application started." << std::endl;

    float target_fps = 60.f;
    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps));

    nanoClock frame_clock;
    frame_clock.restart();

	while(is_running_)
	{
		// Start clock
		// For each layer, update
		// Update window

        auto frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

		WLOGGER.flush();
	}

    DLOG("application",1) << "Application stopped." << std::endl;

    WLOGGER.kill();
}


} // namespace erwin