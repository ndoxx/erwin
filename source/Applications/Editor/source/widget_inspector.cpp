#include "widget_inspector.h"
#include "scene.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

InspectorWidget::InspectorWidget(Scene& scene):
Widget("Inspector", true),
scene_(scene)
{

}

InspectorWidget::~InspectorWidget()
{

}

void InspectorWidget::on_render()
{

}


} // namespace editor