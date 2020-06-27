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

struct SceneHierarchyWidget::SetHierarchyCommand
{
    EntityID source = k_invalid_entity_id;
    EntityID target = k_invalid_entity_id;
};

SceneHierarchyWidget::SceneHierarchyWidget():
Widget("Hierarchy", true)
{

}

static void entity_context_menu(Scene& scene, EntityID e)
{
    ImGui::PushID(int(ImGui::GetID(reinterpret_cast<void*>(intptr_t(e)))));
    if(ImGui::BeginPopupContextItem("Entity context menu"))
    {
        if(!scene.registry.has<NonRemovableTag>(e) && ImGui::Selectable("Remove"))
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
    scene.registry.view<ComponentDescription>(entt::exclude<ComponentHierarchy>).each([&new_selection, &scene](auto e, const auto& desc)
    {
        ImGuiTreeNodeFlags flags = s_base_flags | s_leaf_flags;
        if(scene.registry.has<SelectedTag>(e))
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::TreeNodeEx(reinterpret_cast<void*>(intptr_t(e)), flags, "%s %s", desc.icon.c_str(), desc.name.c_str());

        if(ImGui::IsItemClicked())
            new_selection = e;

        // Disable drag and drop for entities with FixedHierarchyTag
        if(!scene.registry.has<FixedHierarchyTag>(e) && ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("DND_HIER_TREE", &e, sizeof(EntityID));
            ImGui::Text("%s", W_ICON(CHILD));
            ImGui::EndDragDropSource();
        }

        // Context menu for entities
        entity_context_menu(scene, e);
    });
    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());

    // Display entities with hierarchy
    // Entities with hierarchy must be in the subtree of the scene root node in order to appear here
    // OPT: Maybe a depth-first traversal of the whole scene is not needed each frame. Fine for now.
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    auto pos_x = ImGui::GetCursorPosX();
    entity::depth_first(scene.root, scene.registry, [&new_selection, &scene, pos_x, this](auto curr, const auto& curr_hier, size_t depth)
    {
        ImGuiTreeNodeFlags flags = s_base_flags;
        if(scene.registry.has<SelectedTag>(curr))
            flags |= ImGuiTreeNodeFlags_Selected;
        if(curr_hier.children == 0)
            flags |= s_leaf_flags;

        // Cant seem to use Indent() properly, so here I go
        ImGui::SetCursorPosX(pos_x + float(depth) * indent_space_);

        const auto& desc = scene.registry.get<ComponentDescription>(curr);
        bool node_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(intptr_t(curr)), flags, "%s %s", desc.icon.c_str(), desc.name.c_str());
        
        if(ImGui::IsItemClicked())
            new_selection = curr;

        if(!scene.registry.has<FixedHierarchyTag>(curr) && ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("DND_HIER_TREE", &curr, sizeof(EntityID));
            ImGui::Text("%s", W_ICON(CHILD));
            ImGui::EndDragDropSource();
        }

        if(ImGui::BeginDragDropTarget())
        {
            ImGuiDragDropFlags target_flags = 0;
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_HIER_TREE", target_flags))
                set_hierarchy_commands_.push_back({*static_cast<EntityID*>(payload->Data), curr});
            ImGui::EndDragDropTarget();
        }

        // Context menu for entities
        entity_context_menu(scene, curr);

        if(node_open && curr_hier.children != 0)
            ImGui::TreePop();

        return !node_open;
    });

    if(new_selection != k_invalid_entity_id)
    {
    	// Update scene selected entity index
    	scene.select(new_selection);
        // Drop gizmo handle selection
        scene.registry.clear<GizmoHandleSelectedTag>();
    }

    // Hierarchy movements
    if(!set_hierarchy_commands_.empty())
    {
        for(auto&& cmd: set_hierarchy_commands_)
        {
            // TODO: For the moment, prevent swapping a parent with its child, maybe allow
            // it later on. Anyway, this situation must be detected.
            if(!entity::subtree_contains(cmd.source, cmd.target, scene.registry))
            {
                DLOG("editor",1) << "Setting entity #" << size_t(cmd.source) << " as a child of #" << size_t(cmd.target) << std::endl;
                entity::attach(cmd.target, cmd.source, scene.registry);
            }
            else
            {
                DLOGW("editor") << "Swapping a parent with one of its children is not implemented yet." << std::endl;
            }
        }

        entity::sort_hierarchy(scene.registry);
        set_hierarchy_commands_.clear();
    }
}


} // namespace editor