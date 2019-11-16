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
#include "render/main_renderer.h"
#include "render/renderer_2d.h"

#include <iostream>

#define MR__

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
    if(cfg::get<bool>("erwin.logger.track_window_close_events"_h, false))
    {
        WLOGGER.track_event<WindowCloseEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_window_resize_events"_h, false))
    {
        WLOGGER.track_event<WindowResizeEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_framebuffer_resize_events"_h, false))
    {
        WLOGGER.track_event<FramebufferResizeEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_keyboard_events"_h, false))
    {
        WLOGGER.track_event<KeyboardEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_mouse_button_events"_h, false))
    {
        WLOGGER.track_event<MouseButtonEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_mouse_scroll_events"_h, false))
    {
        WLOGGER.track_event<MouseMovedEvent>();
    }
    if(cfg::get<bool>("erwin.logger.track_mouse_moved_events"_h, false))
    {
        WLOGGER.track_event<MouseScrollEvent>();
    }

    WLOGGER.set_single_threaded(cfg::get<bool>("erwin.logger.single_threaded"_h, true));
    WLOGGER.set_backtrace_on_error(cfg::get<bool>("erwin.logger.backtrace_on_error"_h, true));

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
    WindowProps props
    {
        "ErwinEngine",
        cfg::get<uint32_t>("erwin.display.width"_h,  1280),
        cfg::get<uint32_t>("erwin.display.height"_h, 1024),
        cfg::get<bool>("erwin.display.full"_h,       false),
        cfg::get<bool>("erwin.display.topmost"_h,    false),
        cfg::get<bool>("erwin.display.vsync"_h,      true)
    };
    window_ = Window::create(props);

#ifdef MR__
    // Initialize framebuffer pool
    FramebufferPool::init(window_->get_width(), window_->get_height());
    // Initialize master renderer storage
    MainRenderer::init();
    MainRenderer::create_queue(0, SortKey::Order::ByDepthDescending); // Opaque 2D
    MainRenderer::create_queue(1, SortKey::Order::Sequential); // Presentation
    Renderer2D::init();
#endif

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

    float target_fps = cfg::get<uint32_t>("erwin.display.target_fps"_h, 60);
    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps));

    nanoClock frame_clock;
    frame_clock.restart();

	std::chrono::nanoseconds frame_d(16666666);
	while(is_running_)
	{
	    if(game_clock_.is_paused())
	        continue;

	    game_clock_.update(frame_d);

		// For each layer, update
		if(!minimized_)
		{
			for(auto* layer: layer_stack_)
				layer->update(game_clock_);
		}
#ifdef MR__
        MainRenderer::flush();
#endif
		// TODO: move this to render thread when we have one
		IMGUI_LAYER->begin();
		for(auto* layer: layer_stack_)
			layer->on_imgui_render();
		IMGUI_LAYER->end();

        if(!window_->is_vsync())
        {
            auto active_d = frame_clock.get_elapsed_time();
            auto idle_duration = frame_duration_ns_ - active_d;
            std::this_thread::sleep_for(idle_duration);
        }

    	// To allow frame by frame update
    	game_clock_.release_flags();

		// Update window
        window_->update();

		WLOGGER.flush();
        frame_d = frame_clock.restart();
	}

    layer_stack_.clear();

    DLOG("application",1) << WCC(0,153,153) << "--- Application stopped ---" << std::endl;

#ifdef MR__
    // FramebufferPool::shutdown();
    Renderer2D::shutdown();
    MainRenderer::shutdown();
#endif
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