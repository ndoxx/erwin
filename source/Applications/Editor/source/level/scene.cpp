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
    // TMP stub -> Implement proper scene loading

    // Reserve a new asset registry for this scene
    asset_registry_ = AssetManager::create_asset_registry();

    // * Create entities with no async dependency
    // Create root node
    root = create_entity("__root__", W_ICON(CODE_FORK));
    registry.emplace<ComponentTransform3D>(root, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    registry.emplace<FixedHierarchyTag>(root);
    registry.emplace<NonEditableTag>(root);
    registry.emplace<NonRemovableTag>(root);
    registry.emplace<NonSerializableTag>(root);

    {
        EntityID ent = create_entity("Camera", W_ICON(VIDEO_CAMERA));

        registry.emplace<ComponentCamera3D>(ent);
        registry.emplace<ComponentTransform3D>(ent, glm::vec3(-5.8f, 2.3f, -5.8f), glm::vec3(5.f, 228.f, 0.f), 1.f);
        registry.emplace<NoGizmoTag>(ent);
        registry.emplace<FixedHierarchyTag>(ent); // This entity should not be moved in the hierarchy
        camera = ent;
    }

    {
        EntityID ent = create_entity("Sun", W_ICON(SUN_O));

        Material mat_sun;
        mat_sun.archetype = "Sun"_h;
        mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
        mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
        mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
        Renderer3D::register_shader(mat_sun.shader);
        Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

        ComponentDirectionalLight cdirlight;
        cdirlight.set_position(47.626f, 49.027f);
        cdirlight.color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_strength = 0.3f;
        cdirlight.brightness = 3.7f;

        ComponentDirectionalLightMaterial renderable;
        renderable.set_material(mat_sun);
        renderable.material_data.scale = 0.2f;

        registry.emplace<ComponentDirectionalLight>(ent, cdirlight);
        registry.emplace<ComponentDirectionalLightMaterial>(ent, renderable);
        registry.emplace<FixedHierarchyTag>(ent); // This entity should not be moved in the hierarchy

        directional_light = ent;
    }

    // MOCK: load asset registry
    std::vector<hash_t> future_materials = {
        AssetManager::load_async<ComponentPBRMaterial>(asset_registry_,
                                                       project::asset_path(DK::MATERIAL, "greasyMetal.tom")),
        AssetManager::load_async<ComponentPBRMaterial>(asset_registry_,
                                                       project::asset_path(DK::MATERIAL, "scuffedPlastic.tom")),
        AssetManager::load_async<ComponentPBRMaterial>(asset_registry_,
                                                       project::asset_path(DK::MATERIAL, "paintPeelingConcrete.tom")),
        AssetManager::load_async<ComponentPBRMaterial>(asset_registry_,
                                                       project::asset_path(DK::MATERIAL, "dirtyWickerWeave.tom")),
    };

    EntityID sphere0 = create_entity("Sphere #0");
    registry.emplace<ComponentMesh>(sphere0, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere0);
    registry.emplace<ComponentTransform3D>(sphere0, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f), 2.f);

    AssetManager::on_ready<ComponentPBRMaterial>(future_materials[0],
                                                 [this, sphere0 = sphere0](const ComponentPBRMaterial& mat) {
                                                     registry.emplace<ComponentPBRMaterial>(sphere0, mat);
                                                 });

    EntityID sphere1 = create_entity("Sphere #1");
    registry.emplace<ComponentMesh>(sphere1, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere1);
    registry.emplace<ComponentTransform3D>(sphere1, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f), 0.5f);

    AssetManager::on_ready<ComponentPBRMaterial>(future_materials[1],
                                                 [this, sphere1 = sphere1](const ComponentPBRMaterial& mat) {
                                                     registry.emplace<ComponentPBRMaterial>(sphere1, mat);
                                                 });

    EntityID sphere2 = create_entity("Sphere #2");
    registry.emplace<ComponentMesh>(sphere2, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere2);
    registry.emplace<ComponentTransform3D>(sphere2, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f), 0.5f);

    AssetManager::on_ready<ComponentPBRMaterial>(future_materials[2],
                                                 [this, sphere2 = sphere2](const ComponentPBRMaterial& mat) {
                                                     registry.emplace<ComponentPBRMaterial>(sphere2, mat);
                                                 });

    entity::attach(root, sphere0, registry);
    entity::attach(sphere0, sphere1, registry);
    entity::attach(sphere1, sphere2, registry);
    entity::sort_hierarchy(registry);

    load_hdr_environment(project::asset_path(DK::HDR, "small_cathedral_2k.hdr"));

    // * Launch async loading operations
    AssetManager::launch_async_tasks();

    return true;
}

void Scene::on_unload()
{
    directional_light = k_invalid_entity_id;
    camera = k_invalid_entity_id;
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
    scene_f.add_attribute(env_node, "id", std::to_string(environment.resource_id).c_str());

    // Visit each entity, for each component invoke serialization method
    auto* entities_node = scene_f.add_node(scene_f.root, "Entities");
    registry.each([this, &scene_f, entities_node](const EntityID e) {
        if(registry.has<NonSerializableTag>(e))
            return;

        auto* enode = scene_f.add_node(entities_node, "Entity");
        scene_f.add_attribute(enode, "id", std::to_string(size_t(e)).c_str());

        if(auto* p_hier = registry.try_get<ComponentHierarchy>(e))
            scene_f.add_attribute(enode, "parent", std::to_string(size_t(p_hier->parent)).c_str());

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
    DLOGN("editor") << "Deserializing scene: " << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    // TMP
    {
        on_unload();
        asset_registry_ = AssetManager::create_asset_registry();
    }

    // Parse XML file
    xml::XMLFile scene_f(file_path);
    if(!scene_f.read())
    {
        DLOGE("editor") << "Cannot parse scene file." << std::endl;
        return;
    }

    // Create root node
    root = create_entity("__root__", W_ICON(CODE_FORK));
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
            environment = env;
            Renderer3D::set_environment(environment);
            Renderer3D::enable_IBL(true);
        });
    }

    // Load entities
    std::map<size_t, EntityID> id_to_ent_id;
    id_to_ent_id[0] = root;
    std::map<EntityID, size_t> parent_map;
    {
        auto* entities_node = scene_f.root->first_node("Entities");
        W_ASSERT(entities_node, "No <Entities> node.");
        for(auto* entity_node = entities_node->first_node("Entity"); entity_node;
            entity_node = entity_node->next_sibling("Entity"))
        {
            size_t id;
            xml::parse_attribute(entity_node, "id", id);
            auto e = registry.create();
            id_to_ent_id[id] = e;
            size_t parent = 0;
            xml::parse_attribute(entity_node, "parent", parent);
            parent_map[e] = parent;

            // Deserialize components
            for(auto* cmp_node = entity_node->first_node(); cmp_node; cmp_node = cmp_node->next_sibling())
            {
                const uint32_t reflected_type = entt::hashed_string{cmp_node->name()};
                invoke(W_METAFUNC_DESERIALIZE_XML, reflected_type, static_cast<void*>(cmp_node), e, &registry);
            }
        }
    }

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
        environment = env;
        Renderer3D::set_environment(environment);
        Renderer3D::enable_IBL(true);
    });
}

void Scene::add_entity(EntityID entity, const std::string& name, const char* _icon)
{
    ComponentDescription desc = {name, (_icon) ? _icon : W_ICON(CUBE), ""};
    registry.emplace<ComponentDescription>(entity, desc);

    DLOG("editor", 1) << "[Scene] Added entity: " << name << std::endl;
}

void Scene::select(EntityID entity)
{
    registry.clear<SelectedTag>();
    registry.emplace<SelectedTag>(entity);
}

void Scene::drop_selection() { registry.clear<SelectedTag>(); }

void Scene::mark_for_removal(EntityID entity, uint32_t reflected_component)
{
    removed_components_.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity) { removed_entities_.push(entity); }

} // namespace editor