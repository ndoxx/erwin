#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "core/core.h"
#include "core/game_clock.h"
#include "core/layer_stack.h"
#include "core/window.h"
#include <kibble/memory/heap_area.h>
#include <kibble/filesystem/filesystem.h>

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
    inline kb::kfs::FileSystem& get_filesystem() { return filesystem_; }

    bool on_window_close_event(const WindowCloseEvent& e);

    inline void set_on_imgui_newframe_callback(std::function<void(void)> callback) { on_imgui_new_frame_ = callback; }
    bool mirror_settings(const std::string& user_path, const std::string& default_path);

protected:
    bool vsync_enabled_;

private:
    static Application* pinstance_;
    ApplicationParameters parameters_;
    WScope<Window> window_;
    bool is_running_;
    bool minimized_;

    LayerStack layer_stack_;
    GameClock game_clock_;
    kb::kfs::FileSystem filesystem_;

    std::function<void(void)> on_imgui_new_frame_ = []() {};
};

[[maybe_unused]] static inline Application& APP() { return Application::get_instance(); }
[[maybe_unused]] static inline kb::kfs::FileSystem& WFS() { return Application::get_instance().get_filesystem(); }

// Defined in the client
extern Application* create_application();

} // namespace erwin