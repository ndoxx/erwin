#pragma once

#include <string>

#include "core/core.h"
#include "event/window_events.h"
#include "render/gfx_context.h"

namespace erwin
{

struct WindowProps
{
	std::string title   = "ErwinEngine";
	unsigned int width  = 1280;
	unsigned int height = 1024;
	bool full_screen    = false;
	bool always_on_top  = false;
	bool vsync          = true;
};

// Abstract class for window handling
class W_API Window
{
public:
	virtual ~Window() { delete context_; }

	virtual void update() = 0;
	virtual uint32_t get_width() const = 0; 
	virtual uint32_t get_height() const = 0;

	virtual void set_vsync(bool value) = 0;
	virtual bool is_vsync() = 0;

	virtual void* get_native() const = 0;

	static Window* create(const WindowProps& props = WindowProps());

protected:
	GFXContext* context_ = nullptr;
};


} // namespace erwin