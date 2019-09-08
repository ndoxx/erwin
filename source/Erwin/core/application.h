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

private:
	std::unique_ptr<Window> window_;
	bool is_running_;
};

// Defined in the client
Application* create_application();

} // namespace erwin