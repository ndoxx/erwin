#pragma once

#include <vector>
#include <ostream>

#include "core/layer.h"

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

	void commit();

	void clear();

	void set_layer_enabled(size_t index, bool value);

	friend std::ostream& operator <<(std::ostream& stream, const LayerStack& rhs);

	inline std::size_t size() const { return layers_.size(); }
	inline std::vector<Layer*>::iterator begin() { return layers_.begin(); }
	inline std::vector<Layer*>::iterator end()   { return layers_.end(); }

private:
	void update_layer_ids();

private:
	std::vector<Layer*> layers_;
	size_t overlay_pos_ = 0;
};

} // namespace erwin