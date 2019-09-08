#pragma once

#include <memory>
#include "../Erwin/core/window.h"

namespace erwin
{

class GLFWWindow: public Window
{
public:
	GLFWWindow(const WindowProps& props);
	~GLFWWindow();

	virtual void update() override;
	virtual uint32_t get_width() const override; 
	virtual uint32_t get_height() const override;

	virtual void set_vsync(bool value) override;
	virtual bool is_vsync() override;

	virtual void* get_native() const override;

private:
	void init(const WindowProps& props);
	void set_event_callbacks();
	void cleanup();

private:
	struct GLFWWindowDataImpl;
	std::unique_ptr<GLFWWindowDataImpl> data_;
};


} // namespace erwin