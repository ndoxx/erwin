#pragma once

#include "widget/widget.h"

namespace erwin
{
class Scene;
}

namespace erwin
{
class EntityManager;
}

namespace editor
{

class InspectorWidget : public Widget
{
public:
    InspectorWidget(erwin::EventBus&);
    virtual ~InspectorWidget() = default;

protected:
    virtual void on_imgui_render() override;
    void entity_tab();
    void environment_tab();
};

} // namespace editor