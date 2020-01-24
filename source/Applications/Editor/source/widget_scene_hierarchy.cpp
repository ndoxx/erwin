#include "widget_scene_hierarchy.h"
#include "game/scene.h"
#include "erwin.h"
#include "font_awesome4.h"
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
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet

    int node_clicked = -1;
    for(int ii=0; ii<scene_.entities.size(); ++ii)
    {
    	const game::EntityDescriptor& desc = scene_.entities[ii];

    	ImGuiTreeNodeFlags flags = node_flags;
    	if(ii == scene_.selected_entity_idx)
    		flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::TreeNodeEx((void*)(intptr_t)ii, flags, "%s %s", desc.icon, desc.name.c_str());
		if(ImGui::IsItemClicked())
			node_clicked = ii;
    }
    ImGui::TreePop();

    if(node_clicked != -1)
    {
    	// Update selection state

    	// Update scene selected entity index
    	scene_.selected_entity_idx = node_clicked;
    }
}


} // namespace editor