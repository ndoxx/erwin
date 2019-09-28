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
	virtual ~Application() = default;

	virtual void on_load() { }

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);
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