#pragma once

#include <vector>
#include <ostream>

#include "layer.h"
#include "../event/event.h"

namespace erwin
{

class LayerStack
{
public:
	LayerStack();
	~LayerStack();

	size_t push_layer(Layer* layer);
	size_t push_overlay(Layer* layer);

	void pop_layer(size_t index);
	void pop_overlay(size_t index);

	void set_layer_enabled(size_t index, bool value);

	friend std::ostream& operator <<(std::ostream& stream, const LayerStack& rhs);

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

        //return handled;
        return false;
	}

	std::vector<Layer*> layers_;
	size_t overlay_pos_;
};


} // namespace erwin