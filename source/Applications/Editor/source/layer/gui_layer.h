#pragma once

#include <vector>
#include "core/layer.h"
#include "widget/widget.h"

namespace editor
{

class GuiLayer: public erwin::Layer
{
public:
	explicit GuiLayer(const std::string& debug_name): erwin::Layer(debug_name) {}
	virtual ~GuiLayer() = default;

	inline void add_widget(Widget* widget) { widgets_.push_back(widget); }
	inline const auto& get_widgets() const { return widgets_; }

protected:
	std::vector<Widget*> widgets_;
};


} // namespace editor