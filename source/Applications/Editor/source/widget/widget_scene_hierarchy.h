#pragma once

#include "widget/widget.h"

namespace erwin
{
class Scene;
}

namespace editor
{

class SceneHierarchyWidget : public Widget
{
public:
    SceneHierarchyWidget(erwin::EventBus&);
    virtual ~SceneHierarchyWidget() = default;

protected:
    virtual void on_imgui_render() override;

private:
    struct SetHierarchyCommand;
    float indent_space_ = 8.f;
    std::vector<SetHierarchyCommand> set_hierarchy_commands_;
};

} // namespace editor