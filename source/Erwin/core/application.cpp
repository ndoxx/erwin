#include "core/application.h"
#include "asset/asset_manager.h"
#include "core/clock.hpp"
#include "core/intern_string.h"
#include "entity/init.h"
#include "event/window_events.h"
#include "imgui/imgui_layer.h"
#include "input/input.h"
#include "level/scene.h"
#include "level/scene_manager.h"
#include "memory/arena.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_2d.h"
#include "render/renderer_3d.h"
#include "render/renderer_pp.h"
#include "render/shader_lang.h"
#include <kibble/logger/dispatcher.h>
#include <kibble/logger/logger.h>
#include <kibble/logger/sink.h>

#include <iostream>

namespace erwin
{

Application* Application::pinstance_ = nullptr;

static ImGuiLayer* IMGUI_LAYER = nullptr;

static struct ApplicationStorage
{
    std::vector<std::string> configuration_files;
    memory::HeapArea client_area;
    memory::HeapArea system_area;
    memory::HeapArea render_area;
} s_storage;

Application::Application(const ApplicationParameters& params)
    : parameters_(params), event_bus_(settings_)
{
    // Create application singleton
    K_ASSERT(!Application::pinstance_, "Application already exists!");
    Application::pinstance_ = this;
    KLOGGER(create_channel("ios", 1));
    // Initialize file system
    auto root_dir = filesystem_.get_self_directory().parent_path();
    if(!filesystem_.setup_settings_directory(params.vendor, params.name, "usr"))
    {
        KLOGE("core") << "Cannot setup application settings directory." << std::endl;
    }
    if(!filesystem_.alias_directory(root_dir, "root"))
    {
        KLOGE("core") << "Cannot alias root directory." << std::endl;
    }
    if(!filesystem_.alias_directory(root_dir / "config", "syscfg"))
    {
        KLOGE("core") << "Cannot alias config directory." << std::endl;
    }
    if(!filesystem_.alias_directory(root_dir / "source/Erwin/assets", "sysres"))
    {
        KLOGE("core") << "Cannot alias system resources directory." << std::endl;
    }
}

void Application::add_configuration(const std::string& filepath)
{
    if(filesystem_.exists(filepath))
        s_storage.configuration_files.push_back(filepath);
    else
    {
        KLOGW("application") << "Unable to find configuration file:" << std::endl;
        KLOGI << "client configuration directory: " << kb::KS_PATH_ << filesystem_.get_settings_directory()
              << std::endl;
        KLOGI << "file path: " << kb::KS_PATH_ << filepath << std::endl;
    }
}

void Application::add_configuration(const std::string& user_path, const std::string& default_path)
{
    if(mirror_settings(user_path, default_path))
        s_storage.configuration_files.push_back(user_path);
    else
    {
        KLOGW("application") << "Unable to find configuration file:" << std::endl;
        KLOGI << "file path: " << kb::KS_PATH_ << user_path << std::endl;
    }
}

bool Application::mirror_settings(const std::string& user_path, const std::string& default_path)
{
    bool has_user = filesystem_.exists(user_path);
    bool has_default = filesystem_.exists(default_path);

    if(!has_default && !has_user)
    {
        KLOGE("config") << "Failed to open user and default files:" << std::endl;
        KLOGI << "User:    " << kb::KS_PATH_ << user_path << std::endl;
        KLOGI << "Default: " << kb::KS_PATH_ << default_path << std::endl;
        return false;
    }

    bool copy_default = (has_default && !has_user);
    // Copy default if more recent
    if(has_default && has_user)
        copy_default = filesystem_.is_older(user_path, default_path);

    if(copy_default)
    {
        KLOG("config", 1) << "Copying default config:" << std::endl;
        KLOGI << "User:    " << kb::KS_PATH_ << user_path << std::endl;
        KLOGI << "Default: " << kb::KS_PATH_ << default_path << std::endl;
        fs::copy_file(filesystem_.regular_path(default_path), filesystem_.regular_path(user_path),
                      fs::copy_options::overwrite_existing);
    }

    return true;
}

memory::HeapArea& Application::get_client_area() { return s_storage.client_area; }

const memory::HeapArea& Application::get_system_area() { return s_storage.system_area; }

const memory::HeapArea& Application::get_render_area() { return s_storage.render_area; }

void Application::init_logger()
{
    for(size_t ii = 0; ii < settings_.get_array_size("erwin.logger.channels"_h); ++ii)
    {
        std::string channel_name = settings_.get<std::string>(su::h_concat("erwin.logger.channels[", ii, "].name"), "");
        size_t verbosity = settings_.get<size_t>(su::h_concat("erwin.logger.channels[", ii, "].verbosity"), 0);
        hash_t hchan = H_(channel_name);
        bool has_channel = false;
        KLOGGER(has_channel(hchan, has_channel));
        if(!has_channel)
        {
            KLOGGER(create_channel(channel_name, uint8_t(verbosity)));
        }
        else
        {
            KLOGGER(set_channel_verbosity(hchan, uint8_t(verbosity)));
        }
    }
    for(size_t ii = 0; ii < settings_.get_array_size("erwin.logger.sinks"_h); ++ii)
    {
        std::string sink_type = settings_.get<std::string>(su::h_concat("erwin.logger.sinks[", ii, "].type"), "");
        std::string sink_name = settings_.get<std::string>(su::h_concat("erwin.logger.sinks[", ii, "].name"), "");
        std::string attachments = settings_.get<std::string>(su::h_concat("erwin.logger.sinks[", ii, "].channels"), "");
        ;
        hash_t htype = H_(sink_type);
        std::unique_ptr<kb::klog::Sink> p_sink = nullptr;
        switch(htype)
        {
        case "ConsoleSink"_h: {
            p_sink = std::make_unique<kb::klog::ConsoleSink>();
            break;
        }
        case "LogFileSink"_h: {
            std::string dest_file =
                settings_.get<std::string>(su::h_concat("erwin.logger.sinks[", ii, "].destination"), "");
            if(!dest_file.empty())
                p_sink = std::make_unique<kb::klog::LogFileSink>(dest_file);
            break;
        }
        case "NetSink"_h: {
            std::string host =
                settings_.get<std::string>(su::h_concat("erwin.logger.sinks[", ii, "].host"), "localhost");
            uint32_t port = settings_.get<uint32_t>(su::h_concat("erwin.logger.sinks[", ii, "].port"), 31337);
            auto net_sink = std::make_unique<kb::klog::NetSink>();
            if(net_sink->connect(host, uint16_t(port)))
                p_sink = std::move(net_sink);
        }
        }
        if(p_sink)
        {
            bool attach_all = !attachments.compare("all");
            if(attach_all)
            {
                KLOGGER(attach_all(sink_name, std::move(p_sink)));
            }
            else
            {
                std::vector<hash_t> chan_hnames;
                kb::su::tokenize(attachments, ',',
                                 [&](const std::string& chan_name) { chan_hnames.push_back(H_(chan_name.c_str())); });
                if(!chan_hnames.empty())
                {
                    KLOGGER(attach(sink_name, std::move(p_sink), chan_hnames));
                }
            }
        }
    }
    KLOGGER(set_backtrace_on_error(settings_.get<bool>("erwin.logger.backtrace_on_error"_h, true)));
}

bool Application::init()
{
    {
        W_PROFILE_SCOPE("Application config")

        // Initialize config
        settings_.load_toml(filesystem_.regular_path("syscfg://erwin.toml"));
        init_logger();

        // Log basic info
        KLOGN("config") << "[Paths]" << std::endl;
        KLOGI << "Executable path:   " << kb::KS_PATH_ << filesystem_.get_self_directory() << kb::KC_ << std::endl;
        KLOGI << "System config dir: " << kb::KS_PATH_ << filesystem_.get_aliased_directory("syscfg"_h) << kb::KC_
              << std::endl;
        KLOGI << "User settings dir: " << kb::KS_PATH_ << filesystem_.get_aliased_directory("usr"_h) << kb::KC_
              << std::endl;

        // Parse intern strings
        istr::init();
        Input::init();
    }

    // Initialize system memory
    {
        W_PROFILE_SCOPE("System memory init")
        KLOGN("application") << "Initializing system memory" << std::endl;
        size_t system_mem_size = settings_.get<size_t>("erwin.memory.system_area"_h, 10_MB);
        if(!s_storage.system_area.init(system_mem_size))
        {
            KLOGF("application") << "Cannot allocate system memory." << std::endl;
            return false;
        }
    }

    // Initialize renderer memory
    {
        W_PROFILE_SCOPE("Renderer memory init")
        KLOGN("application") << "Initializing renderer memory" << std::endl;
        size_t renderer_mem_size = settings_.get<size_t>("erwin.memory.renderer_area"_h, 20_MB);
        if(!s_storage.render_area.init(renderer_mem_size))
        {
            KLOGF("application") << "Cannot allocate renderer memory." << std::endl;
            return false;
        }
    }

    // Configure client
    {
        W_PROFILE_SCOPE("Client configuration parsing")
        KLOGN("config") << "Parsing client configuration" << std::endl;
        on_client_init();
        for(auto&& cfg_file : s_storage.configuration_files)
            settings_.load_toml(filesystem_.regular_path(cfg_file));
    }

    // Initialize client memory
    {
        W_PROFILE_SCOPE("Client memory init")
        KLOGN("application") << "Initializing client memory" << std::endl;
        size_t client_mem_size = settings_.get<size_t>("client.memory.area"_h, 1_MB);
        if(!s_storage.client_area.init(client_mem_size))
        {
            KLOGF("application") << "Cannot allocate client memory." << std::endl;
            return false;
        }
    }

    // Reflect components
    {
        W_PROFILE_SCOPE("Components reflection")
        entity::init_components();
    }

    slang::register_include_directory("sysres://shaders");

    // Create window
    {
        W_PROFILE_SCOPE("Window creation")
        WindowProps props{settings_.get<std::string>("client.display.title"_h, "ErwinEngine"),
                          settings_.get<uint32_t>("client.display.width"_h, 1280),
                          settings_.get<uint32_t>("client.display.height"_h, 1024),
                          settings_.get<bool>("client.display.full"_h, false),
                          settings_.get<bool>("client.display.topmost"_h, false),
                          settings_.get<bool>("client.display.vsync"_h, true),
                          settings_.get<bool>("client.display.host"_h, true)};
#ifdef W_DEBUG
        props.title += " [DEBUG]";
#endif
        window_ = Window::create(event_bus_, props);
        vsync_enabled_ = props.vsync;
    }

    {
        W_PROFILE_SCOPE("Renderer startup")
        FramebufferPool::init(window_->get_width(), window_->get_height(), event_bus_ /*TMP*/);
        Renderer::init(s_storage.render_area);
        AssetManager::create_asset_registry(); // Create engine asset registry (index 0)
        CommonGeometry::init();
        Renderer2D::init();
        Renderer3D::init();
        PostProcessingRenderer::init(event_bus_);
    }

    {
        W_PROFILE_SCOPE("ImGui overlay creation")
        // Generate ImGui overlay
        IMGUI_LAYER = new ImGuiLayer(*this);
        IMGUI_LAYER->on_attach();
    }

    // React to window close events (and shutdown application)
    event_bus_.subscribe(this, &Application::on_window_close_event);

    {
        W_PROFILE_SCOPE("Application load")
        on_load();
        layer_stack_.commit();
    }

    // Show memory content
#ifdef W_DEBUG
    KLOG("memory", 1) << kb::KF_(204, 153, 0) << "--- System memory area ---" << std::endl;
    s_storage.system_area.debug_show_content();
    KLOG("memory", 1) << kb::KF_(204, 153, 0) << "--- Render memory area ---" << std::endl;
    s_storage.render_area.debug_show_content();
    KLOG("memory", 1) << kb::KF_(204, 153, 0) << "--- Client memory area ---" << std::endl;
    s_storage.client_area.debug_show_content();
#endif

    KLOG("application", 1) << kb::KF_(0, 153, 153) << "--- Application base initialized ---" << std::endl;
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
    KLOG("application", 1) << kb::KF_(0, 153, 153) << "--- Application started ---" << std::endl;

    // Display layer stack composition
    KLOG("application", 1) << kb::KF_(204, 0, 204) << "Layer stack composition:" << std::endl;
    KLOG("application", 1) << kb::KF_(204, 0, 204) << layer_stack_ << std::endl;

    // Profiling options
    W_PROFILE_ENABLE_SESSION(settings_.get<bool>("erwin.profiling.runtime_session_enabled"_h, false));

    kb::nanoClock frame_clock;
    frame_clock.restart();

    std::chrono::nanoseconds frame_d(16666666);
    while(is_running_)
    {
        // --- EVENT PHASE ---
        game_clock_.update(frame_d);

        event_bus_.enqueue(BeginFrameEvent());

        // Dispatch queued events
        event_bus_.dispatch();

        // --- UPDATE PHASE ---
        // For each layer, update
        {
            W_PROFILE_SCOPE("Layer updates")
            for(auto* layer : layer_stack_)
                layer->update(game_clock_);
        }

        // Frame config
        Renderer::set_host_window_size(window_->get_width(), window_->get_height());

        // Asset manager sync work
        AssetManager::update();

        // --- RENDER PHASE ---
        // For each layer, render
        if(!minimized_)
        {
            W_PROFILE_SCOPE("Layer render")
            for(auto* layer : layer_stack_)
                layer->render();
        }

        {
#ifndef W_PROFILE_RENDER
            W_PROFILE_SCOPE("Renderer flush")
#endif
            Renderer::flush();
        }

        // TODO: move this to renderer
        {
            W_PROFILE_SCOPE("ImGui render")
            if(IMGUI_LAYER->is_enabled())
            {
                IMGUI_LAYER->begin();
                on_imgui_new_frame_();
                this->on_imgui_render();
                for(auto* layer : layer_stack_)
                    layer->on_imgui_render();
                IMGUI_LAYER->end();
            }
        }

        // --- CLEANUP PHASE ---
        if(SceneManager::has_current())
            SceneManager::get_current().cleanup();

        // Update window
        window_->update();

        {W_PROFILE_SCOPE("Logger flush")}

        frame_d = frame_clock.restart();
    }

    // Save all client configuration files
    for(auto&& cfg_file : s_storage.configuration_files)
    {
        KLOGN("application") << "Saving config file:" << std::endl;
        KLOGI << kb::KS_PATH_ << cfg_file << std::endl;
        settings_.save_toml(filesystem_.regular_path(cfg_file));
    }

    KLOG("application", 1) << kb::KF_(0, 153, 153) << "--- Application stopped ---" << std::endl;
}

bool Application::on_window_close_event(const WindowCloseEvent&)
{
    is_running_ = false;
    return false;
}

} // namespace erwin