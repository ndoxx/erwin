#pragma once

#include "core/layer.h"
#include "widget/widget.h"
#include <kibble/logger/logger.h>
#include <vector>

namespace editor
{

class GuiLayer : public erwin::Layer
{
public:
    explicit GuiLayer(erwin::Application& application, const std::string& debug_name)
        : erwin::Layer(application, debug_name)
    {}
    virtual ~GuiLayer() = default;

    inline void add_widget(Widget* widget) { widgets_.push_back(widget); }
    inline const auto& get_widgets() const { return widgets_; }

    virtual void on_enable() override
    {
        for(Widget* widget : widgets_)
            widget->restore();
    }

    virtual void on_disable() override
    {
        for(Widget* widget : widgets_)
            widget->save_state_and_hide();
    }

protected:
    std::vector<Widget*> widgets_;
};

} // namespace editor