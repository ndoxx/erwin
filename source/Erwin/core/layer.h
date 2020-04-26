#pragma once

#include <string>

#include "event/window_events.h"
#include "core/game_clock.h"

namespace erwin
{

#define REACT(EVENT) virtual bool on_event(const EVENT&) { return false; }

class Layer
{
public:
	friend class LayerStack; // So that LayerStack can acces the protected on_event() overload set

	explicit Layer(const std::string& debug_name): debug_name_(debug_name), enabled_(true) {}
	virtual ~Layer() = default;

	inline const std::string& get_name() const  { return debug_name_; }
	inline void set_enabled(bool value)         { enabled_ = value; }
	inline void toggle()                        { enabled_ = !enabled_; }
	inline void set_layer_id(uint8_t layer_id)  { layer_id_ = layer_id; }
	inline uint8_t get_layer_id() const         { return layer_id_; }
	inline bool is_enabled() const              { return enabled_; }
	inline void update(GameClock& clock)        { if(enabled_) on_update(clock); }
	inline void render()                        { if(enabled_) on_render(); }

	virtual void on_attach() { }
	virtual void on_detach() { }
	virtual void on_imgui_render() { }

protected:
	virtual void on_update(GameClock&) { }
	virtual void on_render() { }

	REACT(KeyboardEvent)
	REACT(KeyTypedEvent)
	REACT(MouseButtonEvent)
	REACT(MouseScrollEvent)
	REACT(MouseMovedEvent)
	REACT(WindowResizeEvent)
	REACT(WindowMovedEvent)

protected:
	std::string debug_name_;
	bool enabled_;
	uint8_t layer_id_;
};

#undef REACT

} // namespace erwin