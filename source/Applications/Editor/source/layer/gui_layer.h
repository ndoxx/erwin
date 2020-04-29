#pragma once

#include <vector>
#include "core/layer.h"
#include "widget/widget.h"
#include "debug/logger.h"

namespace editor
{

class GuiLayer: public erwin::Layer
{
public:
	explicit GuiLayer(const std::string& debug_name): erwin::Layer(debug_name) {}
	virtual ~GuiLayer() = default;

	inline void add_widget(Widget* widget) { widgets_.push_back(widget); }
	inline const auto& get_widgets() const { return widgets_; }

	virtual void set_enabled(bool value) override
	{
		enabled_ = value;

		if(value)
		{
			for(Widget* widget: widgets_)
				widget->restore();
		}
		else
		{
			for(Widget* widget: widgets_)
				widget->save_state_and_hide();
		}
	}


protected:
	std::vector<Widget*> widgets_;
};


} // namespace editor