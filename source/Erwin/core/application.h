#pragma once

#include <memory>
#include <filesystem>

#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/game_clock.h"
#include "memory/heap_area.h"

namespace fs = std::filesystem;

namespace erwin
{

class W_API Application
{
public:
	Application();
	virtual ~Application() = default;

	virtual void on_pre_init() { }
	virtual void on_client_init() { }
	virtual void on_load() { }
	virtual void on_unload() { }
	virtual void on_imgui_render() { }

	size_t push_layer(Layer* layer, bool enabled=true);
	size_t push_overlay(Layer* layer, bool enabled=true);

	inline void set_layer_enabled(size_t index, bool value) { layer_stack_.set_layer_enabled(index, value); }
	void toggle_imgui_layer();

	// Add an XML configuration file to be parsed at the end of init()
	void add_configuration(const fs::path& filepath);
	void add_configuration(const fs::path& user_path, const fs::path& default_path);

	bool init();
	void run();
	void shutdown();

	void enable_vsync(bool value=true);

	static memory::HeapArea& get_client_area();
	static const memory::HeapArea& get_system_area();
	static const memory::HeapArea& get_render_area();

	static inline Application& get_instance() { return *pinstance_; }
	inline const Window& get_window() { return *window_; }

	bool on_window_close_event(const WindowCloseEvent& e);

protected:
	bool vsync_enabled_;

private:
	static Application* pinstance_;
	WScope<Window> window_;
	bool is_running_;
	bool minimized_;

	LayerStack layer_stack_;
	GameClock game_clock_;
};

// Defined in the client
Application* create_application();

} // namespace erwin