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
#include "entity/component/transform.h"
#include "entity/component/tags.h"
#include "entity/tag_components.h"
#include "project/project.h"

#include "filesystem/xml_file.h"

#include <tuple>

using namespace erwin;

namespace editor
{

Scene::Scene()
{
    // Create root node
    root = create_entity("__root__", W_ICON(CODE_FORK));
    registry.emplace<ComponentTransform3D>(root, glm::vec3(0.f), glm::vec3(0.f), 1.f);
    registry.emplace<FixedHierarchyTag>(root);
    registry.emplace<NonEditableTag>(root);
    registry.emplace<NonRemovableTag>(root);
    registry.emplace<NonSerializableTag>(root);

    // Setup registry signal handling
    registry.on_construct<ComponentMesh>().connect<&entt::registry::emplace_or_replace<DirtyOBBTag>>();
    registry.on_construct<ComponentTransform3D>().connect<&entt::registry::emplace_or_replace<DirtyTransformTag>>();
}

bool Scene::on_load()
{
    // TMP stub -> Implement proper scene loading

    // * Create entities with no async dependency
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

    std::vector<hash_t> future_materials = {
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "greasyMetal.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "scuffedPlastic.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "paintPeelingConcrete.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "dirtyWickerWeave.tom"),
    };

    EntityID sphere0 = create_entity("Sphere #0");
    registry.emplace<ComponentMesh>(sphere0, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere0/*, CommonGeometry::get_mesh("icosphere_pbr"_h).extent*/);
    registry.emplace<ComponentTransform3D>(sphere0, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f), 2.f);

    AssetManager::on_material_ready(future_materials[0], [this, sphere0 = sphere0](const ComponentPBRMaterial& mat) {
        registry.emplace<ComponentPBRMaterial>(sphere0, mat);
    });

    EntityID sphere1 = create_entity("Sphere #1");
    registry.emplace<ComponentMesh>(sphere1, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere1/*, CommonGeometry::get_mesh("icosphere_pbr"_h).extent*/);
    registry.emplace<ComponentTransform3D>(sphere1, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f), 0.5f);

    AssetManager::on_material_ready(future_materials[1], [this, sphere1 = sphere1](const ComponentPBRMaterial& mat) {
        registry.emplace<ComponentPBRMaterial>(sphere1, mat);
    });

    EntityID sphere2 = create_entity("Sphere #2");
    registry.emplace<ComponentMesh>(sphere2, CommonGeometry::get_mesh("icosphere_pbr"_h));
    registry.emplace<ComponentOBB>(sphere2/*, CommonGeometry::get_mesh("icosphere_pbr"_h).extent*/);
    registry.emplace<ComponentTransform3D>(sphere2, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f), 0.5f);

    AssetManager::on_material_ready(future_materials[2], [this, sphere2 = sphere2](const ComponentPBRMaterial& mat) {
        registry.emplace<ComponentPBRMaterial>(sphere2, mat);
    });

    entity::attach(root, sphere0, registry);
    entity::attach(sphere0, sphere1, registry);
    entity::attach(sphere1, sphere2, registry);
    entity::sort_hierarchy(registry);

    load_hdr_environment(project::get_asset_path(project::DirKey::HDR) / "small_cathedral_2k.hdr");

    // * Launch async loading operations
    AssetManager::launch_async_tasks();

    return true;
}

void Scene::on_unload()
{
    directional_light = k_invalid_entity_id;
    camera = k_invalid_entity_id;
    registry.clear();
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
                DLOG("editor",1) << "Removing subtree node: " << size_t(child) << std::endl;
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

void Scene::serialize_xml(const fs::path& file_path)
{
    DLOGN("editor") << "Serializing scene: " << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    // Open XML file
    xml::XMLFile scene_f(file_path);
    scene_f.create_root("Scene");

    // Visit each entity, for each component invoke serialization method
    registry.each([this, &scene_f](const EntityID e)
    {
        if(registry.has<NonSerializableTag>(e))
            return;
        
        auto* enode = scene_f.add_node(scene_f.root, "Entity");
        scene_f.add_attribute(enode, "id", std::to_string(size_t(e)).c_str());

        erwin::visit_entity(registry, e, [&scene_f,enode](uint32_t reflected_type, void* data) {
            const char* component_name = entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>();
            auto* cmp_node = scene_f.add_node(enode, component_name);
            invoke(W_METAFUNC_SERIALIZE_XML, reflected_type, std::as_const(data), &scene_f, static_cast<void*>(cmp_node));
        });
    });

    // Write file
    scene_f.write();

    DLOGI << "done." << std::endl;
}

void Scene::load_hdr_environment(const fs::path& hdr_file)
{
    hash_t future_env = AssetManager::load_environment_async(hdr_file);
    AssetManager::on_environment_ready(future_env, [this](const Environment& env) {
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