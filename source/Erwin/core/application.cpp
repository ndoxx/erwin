#include "core/application.h"
#include "core/clock.hpp"
#include "core/intern_string.h"
#include "core/config.h"
#include "imgui/imgui_layer.h"
#include "input/input.h"
#include "filesystem/filesystem.h"
#include "render/render_device.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "render/renderer_3d.h"
#include "render/renderer_pp.h"
#include "asset/asset_manager.h"
#include "memory/arena.h"
#include "level/scene.h"
#include "entity/init.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"

#include <iostream>

namespace erwin
{

Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

static struct ApplicationStorage
{
    std::vector<fs::path> configuration_files;
    memory::HeapArea client_area;
    memory::HeapArea system_area;
    memory::HeapArea render_area;
} s_storage;

Application::Application():
vsync_enabled_(false),
is_running_(true),
minimized_(false)
{
    // Create application singleton
    W_ASSERT(!Application::pinstance_, "Application already exists!");
	Application::pinstance_ = this;
    // Initialize file system
    filesystem::init();
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

memory::HeapArea& Application::get_client_area()
{
    return s_storage.client_area;
}

const memory::HeapArea& Application::get_system_area()
{
    return s_storage.system_area;
}

const memory::HeapArea& Application::get_render_area()
{
    return s_storage.render_area;
}

bool Application::init()
{
    {
        on_pre_init();

        W_PROFILE_SCOPE("Application config")

        // Initialize config
        cfg::load(filesystem::get_config_dir() / "erwin.xml");

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
        Input::init();
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

    // Configure client
    {
        W_PROFILE_SCOPE("Client configuration parsing")
        DLOGN("config") << "Parsing client configuration" << std::endl;
        on_client_init();
        for(auto&& cfg_file: s_storage.configuration_files)
            cfg::load(cfg_file);
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
    }

    // Reflect components
    {
        W_PROFILE_SCOPE("Components reflection")
        entity::init_components();
    }

    // Create window
    {
        W_PROFILE_SCOPE("Window creation")
        WindowProps props
        {
            cfg::get<std::string>("client.display.title"_h, "ErwinEngine"),
            cfg::get<uint32_t>("client.display.width"_h,    1280),
            cfg::get<uint32_t>("client.display.height"_h,   1024),
            cfg::get<bool>("client.display.full"_h,         false),
            cfg::get<bool>("client.display.topmost"_h,      false),
            cfg::get<bool>("client.display.vsync"_h,        true),
            cfg::get<bool>("client.display.host"_h,         true)
        };
        window_ = Window::create(props);
        vsync_enabled_ = props.vsync;
    }

    {
        W_PROFILE_SCOPE("Renderer startup")
        FramebufferPool::init(window_->get_width(), window_->get_height());
        Renderer::init(s_storage.render_area);
        CommonGeometry::init();
        Renderer2D::init();
        Renderer3D::init();
        PostProcessingRenderer::init();

        // Initialize asset manager
        AssetManager::init(s_storage.client_area);
    }

    {
        W_PROFILE_SCOPE("ImGui overlay creation")
        // Generate ImGui overlay
        IMGUI_LAYER = new ImGuiLayer();
        IMGUI_LAYER->on_attach();
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
        layer_stack_.track_event<WindowMovedEvent>();
    }

    // React to window close events (and shutdown application)
    EventBus::subscribe(this, &Application::on_window_close_event);

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

void Application::shutdown()
{
    {
        W_PROFILE_SCOPE("Layer stack shutdown")
        layer_stack_.clear();
        IMGUI_LAYER->on_detach();
    }
    {
        W_PROFILE_SCOPE("Application unloading")
        on_unload();
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
        Renderer3D::shutdown();
        CommonGeometry::shutdown();
        Renderer::shutdown();
    }
    {
        W_PROFILE_SCOPE("Low level systems shutdown")
        Input::shutdown();
        WLOGGER(kill());
    } 
}

void Application::enable_vsync(bool value)
{
    window_->set_vsync(value);
    vsync_enabled_ = value;
}

size_t Application::push_layer(Layer* layer, bool enabled)
{
    W_PROFILE_FUNCTION()
	size_t index = layer_stack_.push_layer(layer);
    layer->set_enabled(enabled);
	return index;
}

size_t Application::push_overlay(Layer* layer, bool enabled)
{
    W_PROFILE_FUNCTION()
	size_t index = layer_stack_.push_overlay(layer);
    layer->set_enabled(enabled);
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

    nanoClock frame_clock;
    frame_clock.restart();

	std::chrono::nanoseconds frame_d(16666666);
	while(is_running_)
	{
	    if(game_clock_.is_paused())
	        continue;

        // --- EVENT PHASE ---
	    game_clock_.update(frame_d);

        EventBus::enqueue(BeginFrameEvent());

        // Dispatch queued events
        EventBus::dispatch();

        // --- UPDATE PHASE ---
		// For each layer, update
		{
            W_PROFILE_SCOPE("Layer updates")
    		for(auto* layer: layer_stack_)
    			layer->update(game_clock_);
		}

        // Frame config
        Renderer::set_host_window_size(window_->get_width(), window_->get_height());

        // --- RENDER PHASE ---
        // For each layer, render
        if(!minimized_)
        {
            W_PROFILE_SCOPE("Layer render")
            for(auto* layer: layer_stack_)
                layer->render();
        }

        Renderer::flush();

		// TODO: move this to renderer
        {
            W_PROFILE_SCOPE("ImGui render")
            if(IMGUI_LAYER->is_enabled())
            {
        		IMGUI_LAYER->begin();
                this->on_imgui_render();
        		for(auto* layer: layer_stack_)
        			layer->on_imgui_render();
        		IMGUI_LAYER->end();
            }
        }

        // --- CLEANUP PHASE ---
        Scene::cleanup();

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

bool Application::on_window_close_event(const WindowCloseEvent&)
{
	is_running_ = false;
	return false;
}

} // namespace erwin