#pragma once

#include <memory>

#include "core.h"
#include "window.h"
#include "layer_stack.h"

namespace erwin
{

class W_API Application
{
public:
	Application();
	virtual ~Application();

	virtual void on_load() { }

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);
	void run();

	static inline Application& get_instance() { return *pinstance_; }
	inline const Window& get_window() { return *window_; }

	bool on_window_close_event(const WindowCloseEvent& e);

private:
	static Application* pinstance_;
	std::unique_ptr<Window> window_;
	bool is_running_;

	LayerStack layer_stack_;
};

// Defined in the client
Application* create_application();

} // namespace erwin