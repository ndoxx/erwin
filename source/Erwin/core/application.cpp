#include "application.h"
#include "clock.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "imgui/imgui_layer.h"

namespace erwin
{

Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

Application::Application():
window_(std::unique_ptr<Window>(Window::create())),
is_running_(true)
{
	if(Application::pinstance_)
	{
		DLOGF("application") << "Application already exists!" << std::endl;
		fatal();
	}

	Application::pinstance_ = this;
	IMGUI_LAYER = new ImGuiLayer();

	push_overlay(IMGUI_LAYER);

	EVENTBUS.subscribe(this, &Application::on_window_close_event);

	layer_stack_.track_event<KeyboardEvent>();
	layer_stack_.track_event<KeyTypedEvent>();
	layer_stack_.track_event<MouseButtonEvent>();
	layer_stack_.track_event<MouseScrollEvent>();
	layer_stack_.track_event<MouseMovedEvent>();
	layer_stack_.track_event<WindowResizeEvent>();
}

Application::~Application()
{
	//EventBus::Kill(); // Can segfault
}

size_t Application::push_layer(Layer* layer)
{
	size_t index = layer_stack_.push_layer(layer);
	//DLOGR("core") << layer_stack_ << std::endl;
	return index;
}

size_t Application::push_overlay(Layer* layer)
{
	size_t index = layer_stack_.push_overlay(layer);
	//DLOGR("core") << layer_stack_ << std::endl;
	return index;
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
    WLOGGER.track_event<KeyboardEvent>();
    WLOGGER.track_event<MouseButtonEvent>();
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
		for(auto* layer: layer_stack_)
			layer->update();

		// TODO: move this to render thread when we have one
		IMGUI_LAYER->begin();
		for(auto* layer: layer_stack_)
			layer->on_imgui_render();
		IMGUI_LAYER->end();

        auto frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

		// Update window
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