#include "application.h"
#include "clock.hpp"
#include "../debug/logger.h"
#include "../debug/logger_thread.h"

namespace erwin
{


Application::Application():
window_(std::unique_ptr<Window>(Window::create())),
is_running_(true)
{
	EVENTBUS.subscribe(this, &Application::on_window_close_event);
}

Application::~Application()
{
	//EventBus::Kill(); // Can segfault
}

void Application::run()
{
	// Initialize logger
    WLOGGER.create_channel("application", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach_all("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"));
    // WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), {"event"_h});
    WLOGGER.set_backtrace_on_error(true);

    WLOGGER.track_event<WindowCloseEvent>();
    WLOGGER.track_event<WindowResizeEvent>();
    WLOGGER.track_event<KeyPressedEvent>();
    WLOGGER.track_event<KeyReleasedEvent>();
    WLOGGER.track_event<MouseButtonPressedEvent>();
    WLOGGER.track_event<MouseButtonReleasedEvent>();
    //WLOGGER.track_event<MouseMovedEvent>();
    WLOGGER.track_event<MouseScrollEvent>();

    WLOGGER.spawn();

    DLOG("application",1) << "Application started." << std::endl;

    float target_fps = 60.f;
    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps));

    nanoClock frame_clock;
    frame_clock.restart();

	while(is_running_)
	{
		// For each layer, update
		// Update window

        auto frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

        window_->update();

		WLOGGER.flush();
	}

    DLOG("application",1) << "Application stopped." << std::endl;

    WLOGGER.kill();
}

bool Application::on_window_close_event(const WindowCloseEvent& e)
{
	is_running_ = false;
	return false;
}


} // namespace erwin