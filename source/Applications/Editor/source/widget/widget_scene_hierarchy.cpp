#include "widget/widget_scene_hierarchy.h"
#include "level/scene.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
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
    auto& scene = scn::current<EdScene>();
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
        if(e == scene.selected_entity)
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
    	// Update selection state

    	// Update scene selected entity index
    	scene.select(new_selection);
    }
}


} // namespace editor