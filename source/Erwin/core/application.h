#pragma once

#include "core.h"

namespace erwin
{

class W_API Application
{
public:
	Application();
	virtual ~Application();

	void run();

private:
	bool is_running_;
};

// Defined in the client
Application* create_application();

} // namespace erwin