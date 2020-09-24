#pragma once

#include <string>

#include "event/event_bus.h"
#include "core/game_clock.h"

namespace erwin
{

class Layer
{
public:
	friend class LayerStack; // So that LayerStack can acces the protected on_event() overload set

	explicit Layer(const std::string& debug_name): debug_name_(debug_name), enabled_(true), layer_id_(0) {}
	virtual ~Layer() = default;

	inline const std::string& get_name() const  { return debug_name_; }
	inline void toggle()                        { enabled_ = !enabled_; }
	inline void set_layer_id(uint8_t layer_id)  { layer_id_ = layer_id; }
	inline uint8_t get_layer_id() const         { return layer_id_; }
	inline bool is_enabled() const              { return enabled_; }
	inline void update(GameClock& clock)        { if(enabled_) on_update(clock); }
	inline void render()                        { if(enabled_) on_render(); }
	inline void set_enabled(bool value)
	{
		enabled_ = value;
		if(enabled_)
			on_enable();
		else
			on_disable();
	}

	virtual void on_imgui_render() { }

protected:
	virtual void on_attach() { }
	virtual void on_commit() { }
	virtual void on_detach() { }
	virtual void on_update(GameClock&) { }
	virtual void on_render() { }
	virtual void on_enable() { }
	virtual void on_disable() { }

	template <typename ClassT, typename EventT>
	inline void add_listener(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), uint8_t system_id = 0u)
	{
		EventBus::subscribe(instance, memberFunction, subscriber_priority(layer_id_, system_id));
	}

protected:
	std::string debug_name_;
	bool enabled_;
	uint8_t layer_id_;
};

} // namespace erwin