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

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);
	void run();

	bool on_window_close_event(const WindowCloseEvent& e);

private:
	std::unique_ptr<Window> window_;
	bool is_running_;

	LayerStack layer_stack_;
};

// Defined in the client
Application* create_application();

} // namespace erwin