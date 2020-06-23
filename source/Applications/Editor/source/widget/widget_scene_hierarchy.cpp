#include "widget/widget_scene_hierarchy.h"
#include "level/scene.h"
#include "entity/reflection.h"
#include "entity/component/description.h"
#include "entity/tag_components.h"
#include "imgui/font_awesome.h"
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
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;
    
    // Basic controls
    if(ImGui::Button("New entity"))
    {
        // For the moment, create an entity with editor description only
        EntityID ent = scene.registry.create();
        scene.add_entity(ent, "Entity #" + std::to_string(static_cast<unsigned long>(ent)));
    }

    ImGui::Separator();

    // Display hierarchy
    static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGuiTreeNodeFlags node_flags = base_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; // ImGuiTreeNodeFlags_Bullet

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    
    EntityID new_selection = k_invalid_entity_id;

    auto view = scene.registry.view<ComponentDescription>();
    int ii = 0;
    for(const entt::entity e: view)
    {
        const ComponentDescription& desc = view.get<ComponentDescription>(e);

        ImGuiTreeNodeFlags flags = node_flags;
        if(scene.registry.has<SelectedTag>(e))
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::TreeNodeEx(reinterpret_cast<void*>(intptr_t(ii)), flags, "%s %s", desc.icon.c_str(), desc.name.c_str());
        if(ImGui::IsItemClicked())
            new_selection = e;

        // Context menu for entities
        ImGui::PushID(int(ImGui::GetID(reinterpret_cast<void*>(intptr_t(ii)))));
        if(ImGui::BeginPopupContextItem("Entity context menu"))
        {
            if(ImGui::Selectable("Remove"))
            {
                scene.mark_for_removal(e);
                DLOG("editor",1) << "Removed entity " << static_cast<unsigned long>(e) << std::endl;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();

        ++ii;
    }

    if(new_selection != k_invalid_entity_id)
    {
    	// Update scene selected entity index
    	scene.select(new_selection);
        // Drop gizmo handle selection
        scene.registry.clear<GizmoHandleSelectedTag>();
    }
}


} // namespace editor