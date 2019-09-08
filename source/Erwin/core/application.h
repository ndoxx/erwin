#pragma once

#include <memory>

#include "core.h"
#include "window.h"

namespace erwin
{

class W_API Application
{
public:
	Application();
	virtual ~Application();

	void run();

	bool on_window_close_event(const WindowCloseEvent& e);
	bool on_key_pressed_event(const KeyPressedEvent& e);

private:
	std::unique_ptr<Window> window_;
	bool is_running_;
};

// Defined in the client
Application* create_application();

} // namespace erwin