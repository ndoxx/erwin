#include "widget_scene_hierarchy.h"
#include "scene.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

SceneHierarchyWidget::SceneHierarchyWidget(Scene& scene):
Widget("Hierarchy", true),
scene_(scene)
{

}

SceneHierarchyWidget::~SceneHierarchyWidget()
{

}

void SceneHierarchyWidget::on_render()
{

}


} // namespace editor