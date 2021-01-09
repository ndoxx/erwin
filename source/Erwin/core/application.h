#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "core/core.h"
#include "core/game_clock.h"
#include "core/layer_stack.h"
#include "core/window.h"
#include "event/event_bus.h"
#include <kibble/memory/heap_area.h>
#include <kibble/filesystem/filesystem.h>
#include <kibble/config/config.h>

namespace fs = std::filesystem;

namespace erwin
{

struct ApplicationParameters
{
    std::string vendor;
    std::string name;
};

struct WindowCloseEvent;
class W_API Application
{
public:
    Application(const ApplicationParameters& params);
    virtual ~Application() = default;

    virtual void on_client_init() {}
    virtual void on_load() {}
    virtual void on_unload() {}
    virtual void on_imgui_render() {}

    size_t push_layer(Layer* layer, bool enabled = true);
    size_t push_overlay(Layer* layer, bool enabled = true);

    inline void set_layer_enabled(size_t index, bool value) { layer_stack_.set_layer_enabled(index, value); }
    void toggle_imgui_layer();

    // Add an XML configuration file to be parsed at the end of init()
    void add_configuration(const std::string& filepath);
    void add_configuration(const std::string& user_path, const std::string& default_path);

    bool init();
    void run();
    void shutdown();

    void enable_vsync(bool value = true);

    static kb::memory::HeapArea& get_client_area();
    static const kb::memory::HeapArea& get_system_area();
    static const kb::memory::HeapArea& get_render_area();

    static inline Application& get_instance() { return *pinstance_; }
    inline const Window& get_window() { return *window_; }
    inline GameClock& get_clock() { return game_clock_; }
    inline EventBus& get_event_bus() { return event_bus_; }
    inline kb::kfs::FileSystem& get_filesystem() { return filesystem_; }
    inline kb::cfg::Settings& get_settings() { return settings_; }

    bool on_window_close_event(const WindowCloseEvent& e);

    inline void set_on_imgui_newframe_callback(std::function<void(void)> callback) { on_imgui_new_frame_ = callback; }
    bool mirror_settings(const std::string& user_path, const std::string& default_path);

private:
    void init_logger();

protected:
    bool vsync_enabled_ = false;

private:
    bool is_running_ = true;
    bool minimized_ = false;
    ApplicationParameters parameters_;
    kb::cfg::Settings settings_;
    EventBus event_bus_;
    LayerStack layer_stack_;
    GameClock game_clock_;
    kb::kfs::FileSystem filesystem_;
    WScope<Window> window_;

    static Application* pinstance_;
    std::function<void(void)> on_imgui_new_frame_ = []() {};
};

#define APP_ Application::get_instance()
#define CFG_ Application::get_instance().get_settings()
#define WFS_ Application::get_instance().get_filesystem()

// Defined in the client
extern Application* create_application();

} // namespace erwin