#include "core/layer_stack.h"
#include "debug/logger.h"

namespace erwin
{

LayerStack::~LayerStack()
{
	clear();
}

void LayerStack::clear()
{
	for(Layer* layer: layers_)
	{
		layer->on_detach();
		delete layer;
	}
	layers_.clear();
}

size_t LayerStack::push_layer(Layer* layer)
{
	layers_.emplace(layers_.begin() + long(overlay_pos_), layer);
	++overlay_pos_;
	layer->on_attach();

	size_t index = overlay_pos_-1;
	update_layer_ids();

	DLOG("application",1) << "Pushed layer \"" << WCC('n') << layer->get_name() << WCC(0) << "\" at index " << index << std::endl;
	DLOGI << "Overlay position is at: " << overlay_pos_ << std::endl;
	
	return index;
}

size_t LayerStack::push_overlay(Layer* layer)
{
	layers_.emplace_back(layer);
	layer->on_attach();

	size_t index = layers_.size()-1;
	update_layer_ids();

	DLOG("application",1) << "Pushed overlay \"" << WCC('n') << layer->get_name() << WCC(0) << "\" at index " << index << std::endl;

	return index;
}

void LayerStack::pop_layer(size_t index)
{
	if(index < overlay_pos_)
	{
		Layer* layer = layers_.at(index);
		layer->on_detach();
		DLOG("application",1) << "Popped layer \"" << WCC('n') << layer->get_name() << WCC(0) << "\" at index " << index << std::endl;
		
		delete layer;
		layers_.erase(layers_.begin() + long(index));
		--overlay_pos_;

		update_layer_ids();

		DLOGI << "Overlay position is at: " << overlay_pos_ << std::endl;
	}
}

void LayerStack::pop_overlay(size_t index)
{
	if(index >= overlay_pos_)
	{
		Layer* layer = layers_.at(index);
		layer->on_detach();
		DLOG("application",1) << "Popped overlay \"" << WCC('n') << layer->get_name() << WCC(0) << "\" at index " << index << std::endl;

		delete layer;
		layers_.erase(layers_.begin() + long(index));

		update_layer_ids();
	}
}

void LayerStack::update_layer_ids()
{
	uint32_t current_index = 0;
	for(Layer* layer: layers_)
		layer->set_layer_id(uint8_t(current_index++));
}

void LayerStack::set_layer_enabled(size_t index, bool value)
{
	if(index < layers_.size())
		layers_.at(index)->set_enabled(value);
}

std::ostream& operator <<(std::ostream& stream, const LayerStack& rhs)
{
	stream << "[ ";
	for(auto it = rhs.layers_.begin(); it<rhs.layers_.begin()+long(rhs.overlay_pos_); ++it)
		stream << "{" << (*it)->get_name() << "} ";
	stream << "| ";
	for(auto it = rhs.layers_.begin()+long(rhs.overlay_pos_); it<rhs.layers_.end(); ++it)
		stream << "{" << (*it)->get_name() << "} ";
	stream << "]";

	return stream;
}

template <>
bool LayerStack::dispatch<WindowResizeEvent>(const WindowResizeEvent& event)
{
	// bool handled = false;
	for(auto it=layers_.end(); it!=layers_.begin();)
	{
		Layer* layer = *--it;
		if(layer->on_event(event))
		{
			// handled = true;
			break;
		}
	}

    return false;//handled;
}

} // namespace erwin