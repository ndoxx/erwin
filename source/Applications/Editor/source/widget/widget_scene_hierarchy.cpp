#include "widget/widget_scene_hierarchy.h"
#include "level/scene.h"
#include "entity/reflection.h"
#include "entity/component/description.h"
#include "entity/component/hierarchy.h"
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

static void entity_context_menu(Scene& scene, EntityID e)
{
    ImGui::PushID(int(ImGui::GetID(reinterpret_cast<void*>(intptr_t(e)))));
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
}

static ImGuiTreeNodeFlags s_base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
static ImGuiTreeNodeFlags s_leaf_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

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

    // * Display hierarchy
    // Display free entities first

    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
    EntityID new_selection = k_invalid_entity_id;
    scene.registry.view<ComponentDescription>(entt::exclude<HierarchyComponent>).each([&new_selection, &scene](auto e, const auto& desc)
    {
        ImGuiTreeNodeFlags flags = s_base_flags | s_leaf_flags;
        if(scene.registry.has<SelectedTag>(e))
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::TreeNodeEx(reinterpret_cast<void*>(intptr_t(e)), flags, "%s %s", desc.icon.c_str(), desc.name.c_str());
        if(ImGui::IsItemClicked())
            new_selection = e;

        // Context menu for entities
        entity_context_menu(scene, e);
    });

    // Display entities with hierarchy
    // OPT: Maybe a depth-first traversal of the whole scene is not needed each frame
    scene.registry.view<HierarchyComponent>().each([&new_selection, &scene, this](auto e, auto& hier)
    {
        // Root nodes only -> depth-first traversal
        if(hier.parent == k_invalid_entity_id)
        {
            entity::depth_first(e, scene.registry, [&new_selection, &scene, this](auto curr, const auto& curr_hier, size_t depth)
            {
                ImGuiTreeNodeFlags flags = s_base_flags;
                if(scene.registry.has<SelectedTag>(curr))
                    flags |= ImGuiTreeNodeFlags_Selected;
                if(curr_hier.children == 0)
                    flags |= s_leaf_flags;

                ImGui::Indent(float(depth) * indentation_);

                const auto& desc = scene.registry.get<ComponentDescription>(curr);
                bool node_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(intptr_t(curr)), flags, "%s %s", desc.icon.c_str(), desc.name.c_str());
                
                if(ImGui::IsItemClicked())
                    new_selection = curr;

                // Context menu for entities
                entity_context_menu(scene, curr);

                if(node_open && curr_hier.children != 0)
                    ImGui::TreePop();

                return !node_open;
            });
        }
    });


    if(new_selection != k_invalid_entity_id)
    {
    	// Update scene selected entity index
    	scene.select(new_selection);
        // Drop gizmo handle selection
        scene.registry.clear<GizmoHandleSelectedTag>();
    }
}


} // namespace editor