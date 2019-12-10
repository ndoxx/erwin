#pragma once

#include <memory>

#include "core/core.h"
#include "core/window.h"
#include "core/layer_stack.h"
#include "core/game_clock.h"

namespace erwin
{

class W_API Application
{
public:
	Application();
	virtual ~Application();

	virtual void on_client_init() { }
	virtual void on_load() { }
	virtual void on_imgui_render() { }

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);

	inline void set_layer_enabled(size_t index, bool value) { layer_stack_.set_layer_enabled(index, value); }

	// Add an XML configuration file to be parsed at the end of init()
	void add_configuration(const std::string& filename);

	bool init();
	void run();

	static inline Application& get_instance() { return *pinstance_; }
	inline const Window& get_window() { return *window_; }

	bool on_window_close_event(const WindowCloseEvent& e);

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