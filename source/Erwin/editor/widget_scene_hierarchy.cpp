#include "editor/widget_scene_hierarchy.h"
#include "editor/font_awesome.h"
#include "editor/scene.h"
#include "entity/entity_manager.h"
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
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    
    EntityID new_selection = k_invalid_entity_id;
    for(int ii=0; ii<editor::Scene::entities.size(); ++ii)
    {
    	EntityID current_entity = editor::Scene::entities[ii];
        auto& ent = ECS::get_entity(current_entity);

    	ImGuiTreeNodeFlags flags = node_flags;
    	if(current_entity == editor::Scene::selected_entity)
    		flags |= ImGuiTreeNodeFlags_Selected;

		ImGui::TreeNodeEx((void*)(intptr_t)ii, flags, "%s %s", ent.get_icon(), ent.get_name().c_str());
		if(ImGui::IsItemClicked())
			new_selection = current_entity;
    }

    if(new_selection != k_invalid_entity_id)
    {
    	// Update selection state

    	// Update scene selected entity index
    	editor::Scene::select(new_selection);
    }
}


} // namespace editor