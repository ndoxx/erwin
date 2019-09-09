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
	inline bool is_enabled() const { return enabled_; }

	inline void update() { if(enabled_) on_update(); }

	virtual void on_attach() { }
	virtual void on_detach() { }

	REACT(KeyboardEvent)
	REACT(KeyTypedEvent)
	REACT(MouseButtonEvent)
	REACT(MouseScrollEvent)
	REACT(MouseMovedEvent)
	REACT(WindowResizeEvent)

protected:
	virtual void on_update() = 0;

private:
	std::string debug_name_;
	bool enabled_;
};

#undef REACT

} // namespace erwin