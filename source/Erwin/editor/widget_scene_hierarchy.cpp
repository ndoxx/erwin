#include "editor/widget_scene_hierarchy.h"
#include "editor/scene.h"
#include "editor/font_awesome.h"
#include "core/application.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

SceneHierarchyWidget::SceneHierarchyWidget():
Widget("Hierarchy", true)
{

}

SceneHierarchyWidget::~SceneHierarchyWidget()
{

}

void SceneHierarchyWidget::on_imgui_render()
{
    auto& scene = Application::SCENE();

    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    
    int node_clicked = -1;
    for(int ii=0; ii<scene.entities.size(); ++ii)
    {
    	const EntityDescriptor& desc = scene.entities[ii];

    	ImGuiTreeNodeFlags flags = node_flags;
    	if(ii == scene.selected_entity_idx)
    		flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::TreeNodeEx((void*)(intptr_t)ii, flags, "%s %s", desc.icon, desc.name.c_str());
		if(ImGui::IsItemClicked())
			node_clicked = ii;
    }

    if(node_clicked != -1)
    {
    	// Update selection state

    	// Update scene selected entity index
    	scene.selected_entity_idx = node_clicked;
    }
}


} // namespace editor