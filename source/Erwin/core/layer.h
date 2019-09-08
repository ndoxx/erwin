#pragma once

#include <string>

#include "../event/window_events.h"

namespace erwin
{

#define REACT(EVENT) virtual bool on_event(const EVENT& event) { return false; }

class Layer
{
public:
	Layer(const std::string& debug_name);
	virtual ~Layer();

	inline const std::string& get_name() const { return debug_name_; }
	inline void set_enabled(bool value) { enabled_ = value; }

	void update();

	virtual void on_attach() { }
	virtual void on_detach() { }

	REACT(KeyboardEvent)
	REACT(MouseButtonEvent)
	REACT(MouseScrollEvent)
	REACT(WindowResizeEvent)

private:
	virtual void on_update() = 0;

private:
	std::string debug_name_;
	bool enabled_;
};

#undef REACT

} // namespace erwin