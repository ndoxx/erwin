#include "application.h"
#include "core/clock.hpp"
#include "core/intern_string.h"
#include "core/config.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/texture_peek.h"
#include "imgui/imgui_layer.h"
#include "input/input.h"
#include "filesystem/filesystem.h"
#include "render/render_device.h"
#include "render/common_geometry.h"
#include "render/main_renderer.h"
#include "render/renderer_2d.h"
#include "render/renderer_pp.h"
#include "render/renderer_forward.h"
#include "asset/asset_manager.h"
#include "memory/arena.h"

#include <iostream>

namespace erwin
{

// This macro allows us to perform the same action for all event classes.
// When creating a new event type, simply add a line here, that's all.
#define FOR_ALL_EVENTS                      \
        DO_ACTION( WindowCloseEvent )       \
        DO_ACTION( WindowResizeEvent )      \
        DO_ACTION( FramebufferResizeEvent ) \
        DO_ACTION( KeyboardEvent )          \
        DO_ACTION( KeyTypedEvent )          \
        DO_ACTION( MouseButtonEvent )       \
        DO_ACTION( MouseMovedEvent )        \
        DO_ACTION( MouseScrollEvent )



Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

struct ApplicationStorage
{
    std::vector<fs::path> configuration_files;
    memory::HeapArea client_area;
    memory::HeapArea system_area;
    memory::HeapArea render_area;
};
static ApplicationStorage s_storage;

// Helper function to automatically configure an event tracking policy
template <typename EventT>
static inline void configure_event_tracking()
{
    std::string config_key_str = "erwin.events.track." + EventT::NAME;
    if(cfg::get<bool>(H_(config_key_str.c_str()), false))
    {
        WLOGGER(track_event<EventT>());
    }
}

Application::Application():
is_running_(true),
minimized_(false)
{
    // Create application singleton
    W_ASSERT(!Application::pinstance_, "Application already exists!");
	Application::pinstance_ = this;
    // Initialize file system
    filesystem::init();
}

Application::~Application()
{
    {
        W_PROFILE_SCOPE("Layer stack shutdown")
        layer_stack_.clear();
    }
    {
        W_PROFILE_SCOPE("Asset Manager shutdown")
        AssetManager::shutdown();
    }
    {
        W_PROFILE_SCOPE("Renderer shutdown")
        FramebufferPool::shutdown();
        PostProcessingRenderer::shutdown();
        Renderer2D::shutdown();
        ForwardRenderer::shutdown();
        CommonGeometry::shutdown();
        MainRenderer::shutdown();
    }
    {
        W_PROFILE_SCOPE("Low level systems shutdown")
        Input::kill();
        WLOGGER(kill());

        // Shutdown all event pools
        #define DO_ACTION( EVENT_NAME ) EVENTBUS.destroy_event_pool< EVENT_NAME >();
        FOR_ALL_EVENTS
        #undef DO_ACTION

        EventBus::shutdown();
    }
}

void Application::add_configuration(const std::string& filename)
{
    fs::path filepath = filesystem::get_client_config_dir() / filename;
    if(fs::exists(filepath))
        s_storage.configuration_files.push_back(filepath);
    else
    {
        DLOGW("application") << "Unable to find configuration file:" << std::endl;
        DLOGI << "client configuration directory: " << WCC('p') << filesystem::get_client_config_dir() << std::endl;
        DLOGI << "file path: " << WCC('p') << filepath << std::endl;
    }
}

bool Application::init()
{
    {
        W_PROFILE_SCOPE("Application config")

        // Initialize config
        cfg::init(filesystem::get_config_dir() / "erwin.xml");

        // Initialize event bus
        EventBus::init();

        // Log events
        #define DO_ACTION( EVENT_NAME ) configure_event_tracking< EVENT_NAME >();
        FOR_ALL_EVENTS
        #undef DO_ACTION

        WLOGGER(set_single_threaded(cfg::get<bool>("erwin.logger.single_threaded"_h, true)));
        WLOGGER(set_backtrace_on_error(cfg::get<bool>("erwin.logger.backtrace_on_error"_h, true)));

        // Spawn logger thread
        WLOGGER(spawn());
        WLOGGER(sync());

        // Log basic info
        DLOGN("config") << "[Paths]" << std::endl;
        DLOGI << "Executable path: " << WCC('p') << filesystem::get_self_dir() << WCC(0) << std::endl;
        DLOGI << "Root dir:        " << WCC('p') << filesystem::get_root_dir() << WCC(0) << std::endl;
        DLOGI << "Config dir:      " << WCC('p') << filesystem::get_config_dir() << WCC(0) << std::endl;

        // Parse intern strings
        istr::init("intern_strings.txt");
    }

    // Initialize system memory
    {
        W_PROFILE_SCOPE("System memory init")
        DLOGN("application") << "Initializing system memory" << std::endl;
        size_t system_mem_size = cfg::get<size_t>("erwin.memory.system_area"_h, 10_MB);
        if(!s_storage.system_area.init(system_mem_size))
        {
            DLOGF("application") << "Cannot allocate system memory." << std::endl;
            return false;
        }
    }

    // Initialize renderer memory
    {
        W_PROFILE_SCOPE("Renderer memory init")
        DLOGN("application") << "Initializing renderer memory" << std::endl;
        size_t renderer_mem_size = cfg::get<size_t>("erwin.memory.renderer_area"_h, 20_MB);
        if(!s_storage.render_area.init(renderer_mem_size))
        {
            DLOGF("application") << "Cannot allocate renderer memory." << std::endl;
            return false;
        }
    }

    // Initialize system event pools
    {
        W_PROFILE_SCOPE("System event pools init")
        #define DO_ACTION( EVENT_NAME ) \
        { \
            std::string config_key_str = "erwin.memory.event_pool." + EVENT_NAME::NAME; \
            DLOG("memory",1) << "Configuring event pool for " << WCC('n') << EVENT_NAME::NAME << std::endl; \
            EVENTBUS.init_event_pool< EVENT_NAME >(s_storage.system_area, cfg::get<uint32_t>(H_(config_key_str.c_str()), 8)); \
        }

        FOR_ALL_EVENTS
        #undef DO_ACTION
    }

    // Configure client
    {
        W_PROFILE_SCOPE("Client configuration parsing")
        DLOGN("config") << "Parsing client configuration" << std::endl;
        on_client_init();
        for(auto&& cfg_file: s_storage.configuration_files)
            cfg::init_client(cfg_file);
    }

    // Initialize client memory
    {
        W_PROFILE_SCOPE("Client memory init")
        DLOGN("application") << "Initializing client memory" << std::endl;
        size_t client_mem_size = cfg::get<size_t>("client.memory.area"_h, 1_MB);
        if(!s_storage.client_area.init(client_mem_size))
        {
            DLOGF("application") << "Cannot allocate client memory." << std::endl;
            return false;
        }

        // Setup filesystem arena
        filesystem::init_arena(s_storage.client_area, cfg::get<size_t>("client.memory.filesystem.assets"_h, 10_MB));
    }

    // Create window
    {
        W_PROFILE_SCOPE("Window creation")
        WindowProps props
        {
            cfg::get<std::string>("client.display.title"_h, "ErwinEngine"),
            cfg::get<uint32_t>("client.display.width"_h,  1280),
            cfg::get<uint32_t>("client.display.height"_h, 1024),
            cfg::get<bool>("client.display.full"_h,       false),
            cfg::get<bool>("client.display.topmost"_h,    false),
            cfg::get<bool>("client.display.vsync"_h,      true)
        };
        window_ = Window::create(props);
    }

    {
        W_PROFILE_SCOPE("Renderer startup")
        // Initialize framebuffer pool
        FramebufferPool::init(window_->get_width(), window_->get_height());
        // Initialize master renderer storage
        MainRenderer::init(s_storage.render_area);
        // Create common geometry
        CommonGeometry::init();

        MainRenderer::create_queue("Forward3D");
        MainRenderer::create_queue("Sprite2D");
        MainRenderer::create_queue("Blur");
#ifdef W_DEBUG
        MainRenderer::create_queue("Debug2D");
#endif
        MainRenderer::create_queue("Presentation");
#ifdef W_DEBUG
        TexturePeek::init();
#endif
        Renderer2D::init();
        ForwardRenderer::init();
        PostProcessingRenderer::init();

        // Initialize asset manager
        AssetManager::init(s_storage.client_area);
    }

    {
        W_PROFILE_SCOPE("ImGui overlay creation")
        // Generate ImGui overlay
        IMGUI_LAYER = new ImGuiLayer();
        push_overlay(IMGUI_LAYER);
    }

    {
        W_PROFILE_SCOPE("Layer event tracking setup")
        // Propagate input events to layers
        layer_stack_.track_event<KeyboardEvent>();
        layer_stack_.track_event<KeyTypedEvent>();
        layer_stack_.track_event<MouseButtonEvent>();
        layer_stack_.track_event<MouseScrollEvent>();
        layer_stack_.track_event<MouseMovedEvent>();
        layer_stack_.track_event<WindowResizeEvent>();
    }

    // React to window close events (and shutdown application)
    EVENTBUS.subscribe(this, &Application::on_window_close_event);

    {
        W_PROFILE_SCOPE("Application load")
        on_load();
    }

    // Show memory content
#ifdef W_DEBUG
    DLOG("memory",1) << WCC(204,153,0) << "--- System memory area ---" << std::endl;
    s_storage.system_area.debug_show_content();
    DLOG("memory",1) << WCC(204,153,0) << "--- Render memory area ---" << std::endl;
    s_storage.render_area.debug_show_content();
    DLOG("memory",1) << WCC(204,153,0) << "--- Client memory area ---" << std::endl;
    s_storage.client_area.debug_show_content();
#endif

    DLOG("application",1) << WCC(0,153,153) << "--- Application base initialized ---" << std::endl;
    return true;
}

size_t Application::push_layer(Layer* layer)
{
    W_PROFILE_FUNCTION()
	size_t index = layer_stack_.push_layer(layer);
	return index;
}

size_t Application::push_overlay(Layer* layer)
{
    W_PROFILE_FUNCTION()
	size_t index = layer_stack_.push_overlay(layer);
	return index;
}

void Application::toggle_imgui_layer()
{
    if(IMGUI_LAYER)
        IMGUI_LAYER->toggle();
}

void Application::run()
{
    DLOG("application",1) << WCC(0,153,153) << "--- Application started ---" << std::endl;

    // Display layer stack composition
    DLOG("application",1) << WCC(204,0,204) << "Layer stack composition:" << std::endl;
    DLOG("application",1) << WCC(204,0,204) << layer_stack_ << std::endl;

    // Profiling options
    W_PROFILE_ENABLE_SESSION(cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false));

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

        // Dispatch queued events
        EVENTBUS.dispatch();

		// For each layer, update
		{
            W_PROFILE_SCOPE("Layer updates")
    		for(auto* layer: layer_stack_)
    			layer->update(game_clock_);
		}

        // For each layer, render
        if(!minimized_)
        {
            W_PROFILE_SCOPE("Layer render")
            for(auto* layer: layer_stack_)
                layer->render();
        }

#ifdef W_DEBUG
        TexturePeek::render();
#endif

        MainRenderer::flush();
        
		// TODO: move this to renderer
        {
            W_PROFILE_SCOPE("ImGui render")
            if(IMGUI_LAYER->is_enabled())
            {
        		IMGUI_LAYER->begin();
        		for(auto* layer: layer_stack_)
        			layer->on_imgui_render();
                this->on_imgui_render();
        		IMGUI_LAYER->end();
            }
        }

        if(!window_->is_vsync())
        {
            W_PROFILE_SCOPE("Wait")
            auto active_d = frame_clock.get_elapsed_time();
            auto idle_duration = frame_duration_ns_ - active_d;
            std::this_thread::sleep_for(idle_duration);
        }

    	// To allow frame by frame update
    	game_clock_.release_flags();

		// Update window
        window_->update();

        {
            W_PROFILE_SCOPE("Logger flush")
            WLOGGER(flush());
        }

        frame_d = frame_clock.restart();
	}

    DLOG("application",1) << WCC(0,153,153) << "--- Application stopped ---" << std::endl;
}

bool Application::on_window_close_event(const WindowCloseEvent& e)
{
	is_running_ = false;
	return false;
}

} // namespace erwin