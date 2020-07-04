#include "level/scene.h"
#include "asset/asset_manager.h"
#include "debug/logger.h"
#include "entity/component/description.h"
#include "entity/reflection.h"
#include "imgui/font_awesome.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

#include "entity/component/PBR_material.h"
#include "entity/component/bounding_box.h"
#include "entity/component/camera.h"
#include "entity/component/dirlight_material.h"
#include "entity/component/hierarchy.h"
#include "entity/component/light.h"
#include "entity/component/mesh.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "entity/tag_components.h"
#include "project/project.h"

#include "filesystem/xml_file.h"

#include <tuple>

using namespace erwin;

namespace editor
{

Scene::Scene()
{
    // Setup registry signal handling
    registry.on_construct<ComponentMesh>().connect<&entt::registry::emplace_or_replace<DirtyOBBTag>>();
    registry.on_construct<ComponentTransform3D>().connect<&entt::registry::emplace_or_replace<DirtyTransformTag>>();
}

bool Scene::on_load()
{
    asset_registry_ = AssetManager::create_asset_registry();

    return true;
}

void Scene::on_unload()
{
    named_entities_.clear();
    registry.clear();

    AssetManager::release_registry(asset_registry_);
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

        // If entity is named, must remove it from the named entities map
        if(registry.has<NamedEntityTag>(entity))
            if(auto* p_desc = registry.try_get<ComponentDescription>(entity))
                named_entities_.erase(H_(p_desc->name.c_str()));

        // Check if entity has children, if so, the whole subtree must be destroyed
        // ALT: or moved up?
        if(auto* p_hier = registry.try_get<ComponentHierarchy>(entity))
        {
            if(p_hier->parent != k_invalid_entity_id)
                entity::detach(entity, registry);

            std::vector<EntityID> subtree;
            entity::depth_first(entity, registry, [&subtree](EntityID child, const ComponentHierarchy&, size_t) {
                DLOG("editor", 1) << "Removing subtree node: " << size_t(child) << std::endl;
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

void Scene::serialize_xml(const FilePath& file_path)
{
    DLOGN("editor") << "Serializing scene: " << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    // Open XML file
    xml::XMLFile scene_f(file_path);
    scene_f.create_root("Scene");

    // Write resource table
    auto* assets_node = scene_f.add_node(scene_f.root, "Assets");
    const auto& metas = AssetManager::get_resource_meta(asset_registry_);
    for(auto&& [hname, meta] : metas)
    {
        auto* anode = scene_f.add_node(assets_node, "Asset");
        scene_f.add_attribute(anode, "id", std::to_string(hname).c_str());
        scene_f.add_attribute(anode, "type", std::to_string(size_t(meta.type)).c_str());
        scene_f.add_attribute(anode, "path", meta.file_path.file_path().c_str());
    }

    // Write environment
    auto* env_node = scene_f.add_node(scene_f.root, "Environment");
    scene_f.add_attribute(env_node, "id", std::to_string(environment_.resource_id).c_str());

    // Visit each entity, for each component invoke serialization method
    auto* entities_node = scene_f.add_node(scene_f.root, "Entities");
    scene_f.add_attribute(entities_node, "root", std::to_string(size_t(get_named("root"_h))).c_str());

    registry.each([this, &scene_f, entities_node](const EntityID e) {
        if(registry.has<NonSerializableTag>(e))
            return;

        auto* enode = scene_f.add_node(entities_node, "Entity");
        scene_f.add_attribute(enode, "id", std::to_string(size_t(e)).c_str());

        if(auto* p_hier = registry.try_get<ComponentHierarchy>(e))
            scene_f.add_attribute(enode, "parent", std::to_string(size_t(p_hier->parent)).c_str());

        if(registry.has<NamedEntityTag>(e))
            scene_f.add_attribute(enode, "named", "true");

        erwin::visit_entity(registry, e, [&scene_f, enode](uint32_t reflected_type, void* data) {
            const char* component_name = entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>();
            auto* cmp_node = scene_f.add_node(enode, component_name);
            invoke(W_METAFUNC_SERIALIZE_XML, reflected_type, std::as_const(data), &scene_f,
                   static_cast<void*>(cmp_node));
        });
    });

    // Write file
    scene_f.write();

    DLOGI << "done." << std::endl;
}

void Scene::deserialize_xml(const erwin::FilePath& file_path)
{
    DLOGN("editor") << "Loading scene: " << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    // Parse XML file
    xml::XMLFile scene_f(file_path);
    if(!scene_f.read())
    {
        DLOGE("editor") << "Cannot parse scene file." << std::endl;
        return;
    }

    // Create root node
    auto root = create_entity("__root__", W_ICON(CODE_FORK));
    set_named(root, "root"_h);
    registry.emplace<ComponentTransform3D>(root, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    registry.emplace<FixedHierarchyTag>(root);
    registry.emplace<NonEditableTag>(root);
    registry.emplace<NonRemovableTag>(root);
    registry.emplace<NonSerializableTag>(root);

    // Read resource table and load each asset
    const auto& ps = project::get_project_settings();
    auto* assets_node = scene_f.root->first_node("Assets");
    W_ASSERT(assets_node, "No <Assets> node.");
    for(auto* asset_node = assets_node->first_node("Asset"); asset_node; asset_node = asset_node->next_sibling("Asset"))
    {
        size_t sz_asset_type;
        xml::parse_attribute(asset_node, "type", sz_asset_type);
        std::string asset_rel_path;
        xml::parse_attribute(asset_node, "path", asset_rel_path);
        FilePath asset_path(ps.root_folder, asset_rel_path);
        AssetManager::load_resource_async(asset_registry_, AssetMetaData::AssetType(sz_asset_type), asset_path);
    }

    // Load environment
    {
        auto* env_node = scene_f.root->first_node("Environment");
        W_ASSERT(env_node, "No <Environment> node.");
        hash_t id;
        xml::parse_attribute(env_node, "id", id);
        AssetManager::on_ready<Environment>(id, [this](const Environment& env) {
            environment_ = env;
            Renderer3D::set_environment(environment_);
            Renderer3D::enable_IBL(true);
        });
    }

    // Load entities
    std::map<size_t, EntityID> id_to_ent_id;
    std::map<EntityID, size_t> parent_map;
    {
        auto* entities_node = scene_f.root->first_node("Entities");
        size_t root_id = 0;
        xml::parse_attribute(entities_node, "root", root_id);
        id_to_ent_id[root_id] = root;
        W_ASSERT(entities_node, "No <Entities> node.");
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

            // DLOGW("editor") << "Entity #" << size_t(e) << std::endl;
            // Deserialize components
            for(auto* cmp_node = entity_node->first_node(); cmp_node; cmp_node = cmp_node->next_sibling())
            {
                // DLOGW("editor") << ">" << cmp_node->name() << std::endl;
                const uint32_t reflected_type = entt::hashed_string{cmp_node->name()};
                invoke(W_METAFUNC_DESERIALIZE_XML, reflected_type, static_cast<void*>(cmp_node), static_cast<void*>(&registry), e);
            }
        }
    }

    // Register all named entities
    registry.view<ComponentDescription, NamedEntityTag>().each([this](auto e, const auto& desc)
    {
        set_named(e, H_(desc.name.c_str()));
        DLOG("editor",1) << "Registered named entity [" << size_t(e) << "] as " << WCC('n') << desc.name << std::endl;
    });

    // Setup hierarchy
    for(auto&& [e, parent_index] : parent_map)
        entity::attach(id_to_ent_id.at(parent_index), e, registry);
    entity::sort_hierarchy(registry);

    AssetManager::launch_async_tasks();
}

void Scene::load_hdr_environment(const FilePath& hdr_file)
{
    hash_t future_env = AssetManager::load_async<Environment>(asset_registry_, hdr_file);
    AssetManager::on_ready<Environment>(future_env, [this](const Environment& env) {
        environment_ = env;
        Renderer3D::set_environment(environment_);
        Renderer3D::enable_IBL(true);
    });
}

EntityID Scene::create_entity(const std::string& name, const char* _icon)
{
    auto entity = registry.create();
    ComponentDescription desc = {name, (_icon) ? _icon : W_ICON(CUBE), ""};
    registry.emplace<ComponentDescription>(entity, desc);

    DLOG("editor", 1) << "[Scene] Added entity: " << name << std::endl;
    return entity;
}

void Scene::mark_for_removal(EntityID entity, uint32_t reflected_component)
{
    removed_components_.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity) { removed_entities_.push(entity); }

void Scene::set_named(erwin::EntityID ent, erwin::hash_t hname)
{
    W_ASSERT(registry.valid(ent), "[Scene] Invalid entity.");
    named_entities_.insert({hname, ent});
}


} // namespace editor