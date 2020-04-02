#include "editor/widget_scene_hierarchy.h"
#include "editor/font_awesome.h"
#include "level/scene.h"
#include "editor/editor_components.h"
#include "entity/reflection.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

SceneHierarchyWidget::SceneHierarchyWidget():
Widget("Hierarchy", true)
{

}

void SceneHierarchyWidget::on_imgui_render()
{
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    
    EntityID new_selection = k_invalid_entity_id;

    auto view = Scene::registry.view<ComponentEditorDescription>();
    int ii = 0;
    for(const entt::entity e: view)
    {
        const ComponentEditorDescription& desc = view.get<ComponentEditorDescription>(e);

        ImGuiTreeNodeFlags flags = node_flags;
        if(e == Scene::selected_entity)
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::TreeNodeEx((void*)(intptr_t)ii, flags, "%s %s", desc.icon.c_str(), desc.name.c_str());
        if(ImGui::IsItemClicked())
            new_selection = e;

        // Context menu for entities
        ImGui::PushID(ImGui::GetID((void*)(intptr_t)ii));
        if(ImGui::BeginPopupContextItem("Entity context menu"))
        {
            if(ImGui::Selectable("Remove"))
            {
                Scene::mark_for_removal(e);
                DLOG("editor",1) << "Removed entity " << (unsigned long)e << std::endl;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ++ii;
    }

    if(new_selection != k_invalid_entity_id)
    {
    	// Update selection state

    	// Update scene selected entity index
    	Scene::select(new_selection);
    }
}


} // namespace editor