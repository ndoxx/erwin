#pragma once

#include <string>

#include "../core/core.h"

namespace erwin
{

struct WindowProps
{
	std::string title   = "ErwinEngine";
	unsigned int width  = 1024;
	unsigned int height = 768;
	bool full_screen    = false;
	bool always_on_top  = false;
	bool vsync          = true;
};

// Abstract class for window handling
class W_API Window
{
public:
	virtual ~Window() {}

	virtual void update() = 0;
	virtual uint32_t get_width() const = 0; 
	virtual uint32_t get_height() const = 0;

	virtual void set_vsync(bool value) = 0;
	virtual bool is_vsync() = 0;

	virtual void* get_native() const = 0;

	static Window* create(const WindowProps& props = WindowProps());
};


} // namespace erwin