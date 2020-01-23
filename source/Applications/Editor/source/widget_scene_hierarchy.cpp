#include "widget_scene_hierarchy.h"
#include "game/scene.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

SceneHierarchyWidget::SceneHierarchyWidget(game::Scene& scene):
Widget("Hierarchy", true),
scene_(scene)
{

}

SceneHierarchyWidget::~SceneHierarchyWidget()
{

}

void SceneHierarchyWidget::on_imgui_render()
{

}


} // namespace editor