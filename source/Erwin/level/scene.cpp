#include "level/scene.h"
#include "asset/asset_manager.h"
#include <kibble/logger/logger.h>
#include "entity/reflection.h"
#include "imgui/font_awesome.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "script/script_engine.h"

#include "entity/component/description.h"
#include "entity/component/hierarchy.h"
#include "entity/component/mesh.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "entity/component/script.h"
#include "filesystem/xml_file.h"

#include <queue>
#include <stack>
#include <tuple>

namespace erwin
{

void Scene::script_destroy_callback(entt::registry& reg, entt::entity e)
{
    const auto& cscript = reg.get<ComponentScript>(e);
    auto& ctx = ScriptEngine::get_context(script_context_);
    ctx.remove_actor(cscript.actor_index);
}

void Scene::named_tag_destroy_callback(entt::registry& reg, entt::entity e)
{
    if(auto* p_desc = reg.try_get<ComponentDescription>(e))
        named_entities_.erase(H_(p_desc->name.c_str()));
}

Scene::Scene()
{
    // Setup registry signal handling
    registry.on_construct<ComponentMesh>().connect<&entt::registry::emplace_or_replace<DirtyOBBTag>>();
    registry.on_construct<ComponentTransform3D>().connect<&entt::registry::emplace_or_replace<DirtyTransformTag>>();

    // Unload actor on script component destruction
    registry.on_destroy<ComponentScript>().connect<&Scene::script_destroy_callback>(*this);
    // If entity is named, must remove it from the named entities map upon destruction
    registry.on_destroy<NamedEntityTag>().connect<&Scene::named_tag_destroy_callback>(*this);
}

void Scene::unload()
{
    if(!loaded_)
        return;

    named_entities_.clear();
    registry.clear();

    ScriptEngine::destroy_context(script_context_);

    if(!runtime_)
        AssetManager::release_registry(asset_registry_);
    loaded_ = false;
}

void Scene::runtime_clone(const Scene& other)
{
    K_ASSERT(!other.scene_file_path_.empty(), "Cannot runtime clone a scene that hasn't been saved.");

    scene_file_path_ = other.scene_file_path_;
    environment_ = other.environment_;
    inject_ = other.inject_;
    finish_ = other.finish_;
    asset_registry_ = other.asset_registry_;
    runtime_ = true;

    load_xml(scene_file_path_);
}

void Scene::cleanup()
{
    // Removed marked components
    while(!removed_components_.empty())
    {
        auto&& [entity, reflected_component] = removed_components_.front();
        invoke(W_METAFUNC_REMOVE_COMPONENT, reflected_component, registry, entity);
        removed_components_.pop();
    }

    // Removed marked entities and all their components
    while(!removed_entities_.empty())
    {
        auto entity = removed_entities_.front();

        // Check if entity has children, if so, the whole subtree must be destroyed.
        // Can't do this on component destruction as it would put the registry in a
        // bad state, where clearing it causes an assertion in debug. I tried.
        // ALT: or moved up?
        if(auto* p_hier = registry.try_get<ComponentHierarchy>(entity))
        {
            if(p_hier->parent != k_invalid_entity_id)
                detach(entity);

            std::vector<EntityID> subtree;
            depth_first(entity, [&subtree](EntityID child, const ComponentHierarchy&, size_t) {
                KLOG("scene", 1) << "Removing subtree node: " << size_t(child) << std::endl;
                subtree.push_back(child);
                return false;
            });
            registry.destroy(subtree.begin(), subtree.end());
        }
        else
            registry.destroy(entity);

        removed_entities_.pop();
    }
}

void Scene::save()
{
    K_ASSERT(!scene_file_path_.empty(), "Cannot 'save', no output file has been set.");
    K_ASSERT(scene_file_path_.check_extension(".scn"_h), "Only .scn XML files supported for now.");
    save_xml(scene_file_path_);
}

void Scene::save_xml(const WPath& file_path)
{
    KLOGN("scene") << "Serializing scene: " << std::endl;
    KLOGI << kb::KS_PATH_ << file_path << std::endl;

    // * Open XML file
    xml::XMLFile scene_f(file_path);
    scene_f.create_root("Scene");

    // * Write resource table
    auto* assets_node = scene_f.add_node(scene_f.root, "Assets");
    const auto& metas = AssetManager::get_resource_meta(asset_registry_);
    for(auto&& [hname, meta] : metas)
    {
        auto* anode = scene_f.add_node(assets_node, "Asset");
        scene_f.add_attribute(anode, "id", std::to_string(hname).c_str());
        scene_f.add_attribute(anode, "type", std::to_string(size_t(meta.type)).c_str());
        scene_f.add_attribute(anode, "path", meta.file_path.universal().c_str());
    }

    // * Write environment
    auto* env_node = scene_f.add_node(scene_f.root, "Environment");
    scene_f.add_attribute(env_node, "id", std::to_string(environment_.resource_id).c_str());

    // * Write entities
    auto* entities_node = scene_f.add_node(scene_f.root, "Entities");
    scene_f.add_attribute(entities_node, "root", std::to_string(size_t(get_named("root"_h))).c_str());

    // First, get entities in hierarchy order
    // This avoids scene file reordering after each consecutive save, and potential bugs
    std::vector<EntityID> entities;
    registry.view<NamedEntityTag>(entt::exclude<ComponentHierarchy>).each([&entities](auto e)
    {
        entities.push_back(e);
    });
    depth_first(get_named("root"_h), [&entities](EntityID e, const auto&, size_t)
    {
        entities.push_back(e);
        return false;
    });      

    // Visit each entity, for each component invoke serialization method
    for(auto e: entities)
    {
        if(registry.has<NonSerializableTag>(e))
            continue;

        auto* enode = scene_f.add_node(entities_node, "Entity");
        scene_f.add_attribute(enode, "id", std::to_string(size_t(e)).c_str());

        if(auto* p_hier = registry.try_get<ComponentHierarchy>(e))
            scene_f.add_attribute(enode, "parent", std::to_string(size_t(p_hier->parent)).c_str());

        if(registry.has<NamedEntityTag>(e))
            scene_f.add_attribute(enode, "named", "true");

        visit_entity(e, [&scene_f, enode](uint32_t reflected_type, void* data) {
            const char* component_name = entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>();
            auto* cmp_node = scene_f.add_node(enode, component_name);
            invoke(W_METAFUNC_SERIALIZE_XML, reflected_type, std::as_const(data), &scene_f,
                   static_cast<void*>(cmp_node));
        });
    }

    // * Write file
    scene_f.write();

    KLOGI << "done." << std::endl;
}

void Scene::load_xml(const WPath& file_path)
{
    KLOGN("scene") << "Loading scene: " << std::endl;
    KLOGI << kb::KS_PATH_ << file_path << std::endl;

    scene_file_path_ = file_path;
    
    if(!runtime_)
        asset_registry_ = AssetManager::create_asset_registry();

    // Create a script context
    script_context_ = ScriptEngine::create_context(*this);

    // Parse XML file
    xml::XMLFile scene_f(file_path);
    if(!scene_f.read())
    {
        KLOGE("scene") << "Cannot parse scene file." << std::endl;
        return;
    }

    // Create root node
    auto root = create_entity("__root__", W_ICON(CODE_FORK));
    set_named(root, "root"_h);
    registry.emplace<ComponentTransform3D>(root, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    registry.emplace<NonSerializableTag>(root);

    // Read resource table and load each asset
    auto* assets_node = scene_f.root->first_node("Assets");
    K_ASSERT(assets_node, "No <Assets> node.");
    for(auto* asset_node = assets_node->first_node("Asset"); asset_node; asset_node = asset_node->next_sibling("Asset"))
    {
        size_t sz_asset_type;
        xml::parse_attribute(asset_node, "type", sz_asset_type);
        std::string asset_univ_path;
        xml::parse_attribute(asset_node, "path", asset_univ_path);
        AssetManager::load_resource_async(asset_registry_, AssetMetaData::AssetType(sz_asset_type), WPath(asset_univ_path));
    }

    // Load environment
    {
        auto* env_node = scene_f.root->first_node("Environment");
        K_ASSERT(env_node, "No <Environment> node.");
        hash_t id;
        xml::parse_attribute(env_node, "id", id);
        AssetManager::on_ready<Environment>(id, [this](const Environment& env) {
            environment_ = env;
            Renderer3D::set_environment(environment_);
            Renderer3D::enable_IBL(true);
        });
    }

    // Inject entities (editor needs this)
    inject_(*this);

    // Load entities
    std::map<size_t, EntityID> id_to_ent_id;
    std::map<EntityID, size_t> parent_map;
    {
        auto* entities_node = scene_f.root->first_node("Entities");
        size_t root_id = 0;
        xml::parse_attribute(entities_node, "root", root_id);
        id_to_ent_id[root_id] = root;
        K_ASSERT(entities_node, "No <Entities> node.");
        for(auto* entity_node = entities_node->first_node("Entity"); entity_node;
            entity_node = entity_node->next_sibling("Entity"))
        {
            size_t id;
            xml::parse_attribute(entity_node, "id", id);
            EntityID e = registry.create();
            id_to_ent_id[id] = e;
            size_t parent = 0;
            if(xml::parse_attribute(entity_node, "parent", parent))
                parent_map[e] = parent;

            // Is entity named?
            bool is_named = false;
            if(xml::parse_attribute(entity_node, "named", is_named))
                if(is_named)
                    registry.emplace<NamedEntityTag>(e);

            // KLOGW("scene") << "Entity #" << size_t(e) << std::endl;
            // Deserialize components
            for(auto* cmp_node = entity_node->first_node(); cmp_node; cmp_node = cmp_node->next_sibling())
            {
                // KLOGW("scene") << ">" << cmp_node->name() << std::endl;
                const uint32_t reflected_type = entt::hashed_string{cmp_node->name()};
                invoke(W_METAFUNC_DESERIALIZE_XML, reflected_type, static_cast<void*>(cmp_node),
                       static_cast<void*>(this), e);
            }
        }
    }

    // Register all named entities
    registry.view<ComponentDescription, NamedEntityTag>().each([this](auto e, const auto& desc) {
        set_named(e, H_(desc.name.c_str()));
        KLOG("scene", 1) << "Registered named entity [" << size_t(e) << "] as " << kb::KS_NAME_ << desc.name << std::endl;
    });

    // Setup hierarchy
    for(auto&& [e, parent_index] : parent_map)
        attach(id_to_ent_id.at(parent_index), e);
    sort_hierarchy();

    // Call finisher callback
    finish_(*this);

    AssetManager::launch_async_tasks();
    loaded_ = true; // TODO: More granularity. At this stage scene is not FULLY loaded
}

void Scene::create_asset_registry()
{
    // NOTE(ndx): Dangerous, does not check if a registry is already created
    asset_registry_ = AssetManager::create_asset_registry();
}

void Scene::load_hdr_environment(const WPath& hdr_file)
{
    hash_t future_env = AssetManager::load_async<Environment>(asset_registry_, hdr_file);
    AssetManager::on_ready<Environment>(future_env, [this](const Environment& env) {
        // Unload current environment
        if(environment_.resource_id != 0)
            AssetManager::release<Environment>(asset_registry_, environment_.resource_id);

        environment_ = env;
        Renderer3D::set_environment(environment_);
        Renderer3D::enable_IBL(true);
    });
    AssetManager::launch_async_tasks();
}

EntityID Scene::create_entity() { return registry.create(); }

EntityID Scene::create_entity(const std::string& name, const char* _icon)
{
    auto entity = registry.create();
    ComponentDescription desc = {name, (_icon) ? _icon : W_ICON(CUBE), ""};
    registry.emplace<ComponentDescription>(entity, desc);

    KLOG("scene", 1) << "[Scene] Added entity: " << name << std::endl;
    return entity;
}

void Scene::mark_for_removal(EntityID entity, uint32_t reflected_component)
{
    removed_components_.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity) { removed_entities_.push(entity); }

void Scene::set_named(EntityID ent, hash_t hname)
{
    K_ASSERT(registry.valid(ent), "[Scene] Invalid entity.");
    named_entities_.insert({hname, ent});
}

void Scene::attach(EntityID parent, EntityID child)
{
    // Can't seem to use entt::get_or_emplace() here as creating a new component
    // can invalidate previously acquired reference, resulting in a segfault later on
    if(!registry.has<ComponentHierarchy>(parent))
        registry.emplace<ComponentHierarchy>(parent);
    if(!registry.has<ComponentHierarchy>(child))
        registry.emplace<ComponentHierarchy>(child);

    auto& parent_hierarchy = registry.get<ComponentHierarchy>(parent);
    auto& child_hierarchy = registry.get<ComponentHierarchy>(child);

    // Make sure we don't try to attach a node to itself or one of its children
    K_ASSERT_FMT(!subtree_contains(child, parent), "Child node [%zu] cannot be the ancestor of its parent [%zu].",
                 size_t(child), size_t(parent));

    // If child was already assigned a parent, detach it from tree
    if(child_hierarchy.parent != k_invalid_entity_id)
        detach(child);

    child_hierarchy.parent = parent;
    child_hierarchy.next_sibling = parent_hierarchy.first_child;

    if(parent_hierarchy.first_child != k_invalid_entity_id)
    {
        auto& first_sibling = registry.get<ComponentHierarchy>(parent_hierarchy.first_child);
        first_sibling.previous_sibling = child;
    }

    parent_hierarchy.first_child = child;
    ++parent_hierarchy.children;
}

void Scene::detach(EntityID node)
{
    auto& node_hierarchy = registry.get<ComponentHierarchy>(node);
    auto& parent_hierarchy = registry.get<ComponentHierarchy>(node_hierarchy.parent);
    K_ASSERT(node_hierarchy.parent != k_invalid_entity_id, "Cannot detach orphan node.");

    // Stitch back siblings if any
    if(node_hierarchy.next_sibling != k_invalid_entity_id)
    {
        auto& next_sibling = registry.get<ComponentHierarchy>(node_hierarchy.next_sibling);
        next_sibling.previous_sibling = node_hierarchy.previous_sibling;
    }
    if(node_hierarchy.previous_sibling != k_invalid_entity_id)
    {
        auto& previous_sibling = registry.get<ComponentHierarchy>(node_hierarchy.previous_sibling);
        previous_sibling.next_sibling = node_hierarchy.next_sibling;
    }
    // If previous sibling is null, it means that this child is the first child of its parent
    // then, parent needs to be updated too
    else
        parent_hierarchy.first_child = node_hierarchy.next_sibling;

    --parent_hierarchy.children;
    node_hierarchy.parent = k_invalid_entity_id;
    node_hierarchy.previous_sibling = k_invalid_entity_id;
    node_hierarchy.next_sibling = k_invalid_entity_id;
}

void Scene::sort_hierarchy()
{
    registry.sort<ComponentHierarchy>([this](const entt::entity lhs, const entt::entity rhs) {
        const auto& clhs = registry.get<ComponentHierarchy>(lhs);
        const auto& crhs = registry.get<ComponentHierarchy>(rhs);
        return crhs.parent == lhs || clhs.next_sibling == rhs ||
               (!(clhs.parent == rhs || crhs.next_sibling == lhs) &&
                (clhs.parent < crhs.parent || (clhs.parent == crhs.parent && &clhs < &crhs)));
    });
}

using Snapshot = std::pair<EntityID, size_t>; // Store entity along its depth

// ASSUME: hierarchy is a tree.
//         -> nodes are always visited once, no need to check if that's the case
void Scene::depth_first(EntityID node, NodeVisitor visit)
{
    // DynamicSparseSet<size_t> visited; // O(1) search, ideal for this job!
    std::stack<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while(!candidates.empty())
    {
        auto [ent, depth] = candidates.top();
        candidates.pop();

        const auto& hier = registry.get<ComponentHierarchy>(ent);

        // Stack may contain same node twice.
        // if(!visited.has(size_t(ent)))
        // {
        if(visit(ent, hier, depth))
            break;
        //     visited.insert(size_t(ent));
        // }

        // Push all children to the stack
        auto child = hier.first_child;
        while(child != entt::null)
        {
            // if(!visited.has(size_t(child)))
            candidates.push({child, depth + 1});
            child = registry.get<ComponentHierarchy>(child).next_sibling;
        }
    }
}

// ASSUME: hierarchy is a tree.
//         -> nodes are always visited once, no need to check if that's the case
void Scene::depth_first_ordered(EntityID node, NodeVisitor visit)
{
    std::stack<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while(!candidates.empty())
    {
        auto [ent, depth] = candidates.top();
        candidates.pop();

        const auto& hier = registry.get<ComponentHierarchy>(ent);
        if(visit(ent, hier, depth))
            break;

        // Push all children to the stack
        // They must however be pushed in the reverse order they appear for
        // the first child to be visited first during the next iteration
        // std::vector was measured to be faster here than std::stack
        auto child = hier.first_child;
        std::vector<Snapshot> children;
        while(child != entt::null)
        {
            children.push_back({child, depth + 1});
            child = registry.get<ComponentHierarchy>(child).next_sibling;
        }
        for(auto it = children.rbegin(); it != children.rend(); ++it)
            candidates.push(*it);
    }
}

// ASSUME: hierarchy is a tree.
//         -> nodes are always visited once, no need to check if that's the case
void Scene::breadth_first(EntityID node, NodeVisitor visit)
{
    // DynamicSparseSet<size_t> visited;
    std::queue<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while(!candidates.empty())
    {
        auto [ent, depth] = candidates.front();
        candidates.pop();

        const auto& hier = registry.get<ComponentHierarchy>(ent);
        if(visit(ent, hier, depth))
            break;

        // Push all children to the queue
        auto child = hier.first_child;
        while(child != entt::null)
        {
            candidates.push({child, depth + 1});
            child = registry.get<ComponentHierarchy>(child).next_sibling;
        }
    }
}

bool Scene::subtree_contains(EntityID root, EntityID node)
{
    bool found = false;
    depth_first(root, [&found, node](EntityID ent, const auto&, size_t) {
        found |= (ent == node);
        return found;
    });

    return found;
}

bool Scene::is_child(EntityID parent, EntityID node) const
{
    const auto& parent_hierarchy = registry.get<ComponentHierarchy>(parent);

    auto curr = parent_hierarchy.first_child;
    while(curr != entt::null)
    {
        if(node == curr)
            return true;
        curr = registry.get<ComponentHierarchy>(curr).next_sibling;
    }
    return false;
}

bool Scene::is_sibling(EntityID first, EntityID second) const
{
    const auto& hierarchy_1 = registry.get<ComponentHierarchy>(first);
    const auto& hierarchy_2 = registry.get<ComponentHierarchy>(second);
    if(hierarchy_1.parent == k_invalid_entity_id || hierarchy_2.parent == k_invalid_entity_id ||
       hierarchy_1.parent != hierarchy_2.parent)
        return false;

    const auto& parent_hierarchy = registry.get<ComponentHierarchy>(hierarchy_1.parent);

    auto curr = parent_hierarchy.first_child;
    while(curr != entt::null)
    {
        if(second == curr)
            return true;
        curr = registry.get<ComponentHierarchy>(curr).next_sibling;
    }
    return false;
}

} // namespace erwin