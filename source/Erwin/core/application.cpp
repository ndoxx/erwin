#include "application.h"
#include "core/clock.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "imgui/imgui_layer.h"
#include "input/input.h"
#include "core/intern_string.h"
#include "core/file_system.h"
#include "render/render_device.h"

#include "inih/cpp/INIReader.h"

#include <iostream>

namespace erwin
{

Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

Application::Application():
window_(WScope<Window>(Window::create(/*{"ErwinEngine", 1920, 1200, true, false, true}*/))),
is_running_(true),
minimized_(false)
{
	// Create application singleton
	if(Application::pinstance_)
	{
		DLOGF("application") << "Application already exists!" << std::endl;
		fatal();
	}
	Application::pinstance_ = this;

	// Initialize logger
    WLOGGER.create_channel("application", 3);
    WLOGGER.create_channel("render", 3);
    WLOGGER.create_channel("shader", 3);
    WLOGGER.create_channel("texture", 3);
    WLOGGER.create_channel("util", 3);
    WLOGGER.create_channel("config", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach_all("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"));
    // WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), {"event"_h});
    WLOGGER.set_single_threaded(true);
    
    // Initialize file system
    filesystem::init();

	// Read .ini file
	// TMP: move to proper configuration system
	fs::path ini_path = filesystem::get_config_dir() / "erwin.ini";
	INIReader reader(ini_path.string().c_str());
    if(reader.ParseError() < 0)
    {
        std::cout << "Can't load 'erwin.ini'" << std::endl;
    }
    bool lg_backtrace_on_error = reader.GetBoolean("logger", "backtrace_on_error", true);
    bool lg_single_threaded    = reader.GetBoolean("logger", "single_threaded", false);

    WLOGGER.set_single_threaded(lg_single_threaded);
    WLOGGER.set_backtrace_on_error(lg_backtrace_on_error);

    // Log events
    if(reader.GetBoolean("logger", "track_window_close_events", false))
    {
    	WLOGGER.track_event<WindowCloseEvent>();
    }
    if(reader.GetBoolean("logger", "track_window_resize_events", false))
    {
    	WLOGGER.track_event<WindowResizeEvent>();
    }
    if(reader.GetBoolean("logger", "track_framebuffer_resize_events", false))
    {
    	WLOGGER.track_event<FramebufferResizeEvent>();
    }
    if(reader.GetBoolean("logger", "track_keyboard_events", false))
    {
    	WLOGGER.track_event<KeyboardEvent>();
    }
    if(reader.GetBoolean("logger", "track_mouse_button_events", false))
    {
    	WLOGGER.track_event<MouseButtonEvent>();
    }
    if(reader.GetBoolean("logger", "track_mouse_scroll_events", false))
    {
    	WLOGGER.track_event<MouseMovedEvent>();
    }
    if(reader.GetBoolean("logger", "track_mouse_moved_events", false))
    {
    	WLOGGER.track_event<MouseScrollEvent>();
    }

    WLOGGER.spawn();
    WLOGGER.sync();

    // Parse intern strings
    istr::init("intern_strings.txt");

    // Initialize framebuffer pool
    Gfx::create_framebuffer_pool(window_->get_width(), window_->get_height());

    // Generate ImGui overlay
	IMGUI_LAYER = new ImGuiLayer();
	push_overlay(IMGUI_LAYER);

	// Propagate input events to layers
	layer_stack_.track_event<KeyboardEvent>();
	layer_stack_.track_event<KeyTypedEvent>();
	layer_stack_.track_event<MouseButtonEvent>();
	layer_stack_.track_event<MouseScrollEvent>();
	layer_stack_.track_event<MouseMovedEvent>();
	layer_stack_.track_event<WindowResizeEvent>();

	// React to window close events (and shutdown application)
	EVENTBUS.subscribe(this, &Application::on_window_close_event);

	on_load();
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
    DLOG("application",1) << "Application started." << std::endl;

    float target_fps = 60.f;
    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps));

    nanoClock frame_clock;
    frame_clock.restart();

	Gfx::device->set_clear_color(0.2f,0.2f,0.2f,1.f);

	std::chrono::nanoseconds frame_d(16666666);
	while(is_running_)
	{
		Gfx::device->clear(CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG);

	    if(game_clock_.is_paused())
	        continue;

	    game_clock_.update(frame_d);

		// For each layer, update
		if(!minimized_)
		{
			for(auto* layer: layer_stack_)
				layer->update(game_clock_);
		}

		// TODO: move this to render thread when we have one
		IMGUI_LAYER->begin();
		for(auto* layer: layer_stack_)
			layer->on_imgui_render();
		IMGUI_LAYER->end();

        frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

    	// To allow frame by frame update
    	game_clock_.release_flags();

		// Update window
        window_->update();

		WLOGGER.flush();
	}

    DLOG("application",1) << "Application stopped." << std::endl;

    Gfx::framebuffer_pool->release();

    Input::kill();
    WLOGGER.kill();
    EventBus::Kill();
}

bool Application::on_window_close_event(const WindowCloseEvent& e)
{
	is_running_ = false;
	return false;
}

} // namespace erwin