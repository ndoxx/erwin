#pragma once

#include <string>

#include "event/window_events.h"
#include "core/game_clock.h"

namespace erwin
{

#define REACT(EVENT) virtual bool on_event(const EVENT& event) { return false; }

class Layer
{
public:
	Layer(const std::string& debug_name);
	virtual ~Layer();

	inline const std::string& get_name() const  { return debug_name_; }
	inline void set_enabled(bool value)         { enabled_ = value; }
	inline void set_priority(uint32_t priority) { priority_ = priority; }
	inline uint32_t get_priority() const        { return priority_; }
	inline bool is_enabled() const              { return enabled_; }
	inline void update(GameClock& clock)        { if(enabled_) on_update(clock); }

	virtual void on_attach() { }
	virtual void on_detach() { }
	virtual void on_imgui_render() { }

	REACT(KeyboardEvent)
	REACT(KeyTypedEvent)
	REACT(MouseButtonEvent)
	REACT(MouseScrollEvent)
	REACT(MouseMovedEvent)
	REACT(WindowResizeEvent)

protected:
	virtual void on_update(GameClock& clock) = 0;

private:
	std::string debug_name_;
	bool enabled_;
	uint32_t priority_;
};

#undef REACT

} // namespace erwin