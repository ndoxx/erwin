#pragma once

#include <vector>
#include <ostream>

#include "core/layer.h"
#include "event/event_bus.h"

namespace erwin
{

class LayerStack
{
public:
	~LayerStack();

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);

	void pop_layer(size_t index);
	void pop_overlay(size_t index);

	void clear();

	void set_layer_enabled(size_t index, bool value);

	friend std::ostream& operator <<(std::ostream& stream, const LayerStack& rhs);

	inline std::size_t size() const { return layers_.size(); }
	inline std::vector<Layer*>::iterator begin() { return layers_.begin(); }
	inline std::vector<Layer*>::iterator end()   { return layers_.end(); }

	template <typename EventT>
	void track_event()
	{
		EVENTBUS.subscribe(this, &LayerStack::dispatch<EventT>);
	}

private:
	template <typename EventT>
	bool dispatch(const EventT& event)
	{
		bool handled = false;
		for(auto it=layers_.end(); it!=layers_.begin();)
		{
			Layer* layer = *--it;
			if(!layer->is_enabled()) continue;
			if(layer->on_event(event))
			{
				handled = true;
				break;
			}
		}

        return false;//handled;
	}

	void update_layer_ids();

private:
	std::vector<Layer*> layers_;
	size_t overlay_pos_ = 0;
};

// Specialization to force propagation of window resize events even
// if layer is disabled (to avoid shape stretching glitch on layer re-enabling)
template <>
bool LayerStack::dispatch<WindowResizeEvent>(const WindowResizeEvent& event);

} // namespace erwin