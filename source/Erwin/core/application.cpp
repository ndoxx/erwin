#include "application.h"
#include "core/clock.hpp"
#include "core/intern_string.h"
#include "core/config.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "imgui/imgui_layer.h"
#include "input/input.h"
#include "filesystem/filesystem.h"
#include "render/render_device.h"

#include <iostream>

namespace erwin
{

Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

Application::Application():
is_running_(true),
minimized_(false)
{
    // Create application singleton
    W_ASSERT(!Application::pinstance_, "Application already exists!");
	Application::pinstance_ = this;

    // Initialize file system
    filesystem::init();
    // Initialize config
    cfg::init(filesystem::get_config_dir() / "erwin.xml");

    // Log events
    if(cfg::get<bool>("root.logger.track_window_close_events"_h, false))
    {
        WLOGGER.track_event<WindowCloseEvent>();
    }
    if(cfg::get<bool>("root.logger.track_window_resize_events"_h, false))
    {
        WLOGGER.track_event<WindowResizeEvent>();
    }
    if(cfg::get<bool>("root.logger.track_framebuffer_resize_events"_h, false))
    {
        WLOGGER.track_event<FramebufferResizeEvent>();
    }
    if(cfg::get<bool>("root.logger.track_keyboard_events"_h, false))
    {
        WLOGGER.track_event<KeyboardEvent>();
    }
    if(cfg::get<bool>("root.logger.track_mouse_button_events"_h, false))
    {
        WLOGGER.track_event<MouseButtonEvent>();
    }
    if(cfg::get<bool>("root.logger.track_mouse_scroll_events"_h, false))
    {
        WLOGGER.track_event<MouseMovedEvent>();
    }
    if(cfg::get<bool>("root.logger.track_mouse_moved_events"_h, false))
    {
        WLOGGER.track_event<MouseScrollEvent>();
    }

    WLOGGER.set_single_threaded(cfg::get<bool>("root.logger.single_threaded"_h, true));
    WLOGGER.set_backtrace_on_error(cfg::get<bool>("root.logger.backtrace_on_error"_h, true));

    // Spawn logger thread
    WLOGGER.spawn();
    WLOGGER.sync();

    // Log basic info
    DLOGN("config") << "[Paths]" << std::endl;
    DLOGI << "Executable path: " << WCC('p') << filesystem::get_self_dir() << WCC(0) << std::endl;
    DLOGI << "Root dir:        " << WCC('p') << filesystem::get_root_dir() << WCC(0) << std::endl;
    DLOGI << "Config dir:      " << WCC('p') << filesystem::get_config_dir() << WCC(0) << std::endl;

    // Parse intern strings
    istr::init("intern_strings.txt");

    // Initialize window
    window_ = Window::create(/*{"ErwinEngine", 1920, 1200, true, false, true}*/);

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

    DLOG("application",1) << WCC(0,153,153) << "--- Application base initialized ---" << std::endl;
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
    DLOG("application",1) << WCC(0,153,153) << "--- Application started ---" << std::endl;

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

    DLOG("application",1) << WCC(0,153,153) << "--- Application stopped ---" << std::endl;

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