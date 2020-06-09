#include "level/scene.h"
#include "asset/asset_manager.h"
#include "debug/logger.h"
#include "entity/component_description.h"
#include "entity/reflection.h"
#include "imgui/font_awesome.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_camera.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/light.h"
#include "project/project.h"

#include <tuple>

using namespace erwin;

namespace editor
{

bool Scene::on_load()
{
    // TMP stub -> Implement proper scene loading

    AssetManager::load_shader("shaders/deferred_PBR.glsl");
    AssetManager::create_material_data_buffer<ComponentPBRMaterial>();

    // * Create entities with no async dependency
    {
        EntityID ent = create_entity("Camera", W_ICON(VIDEO_CAMERA));

        ComponentTransform3D transform({-5.8f, 2.3f, -5.8f}, {5.f, 228.f, 0.f}, 1.f);

        registry.assign<ComponentCamera3D>(ent);
        registry.assign<ComponentTransform3D>(ent, transform);
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

        registry.assign<ComponentDirectionalLight>(ent, cdirlight);
        registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

        directional_light = ent;
    }

    // * Load all resources asynchronously
    std::vector<hash_t> future_materials = {
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "greasyMetal.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "scuffedPlastic.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "paintPeelingConcrete.tom"),
        AssetManager::load_material_async(project::get_asset_path(project::DirKey::MATERIAL) / "dirtyWickerWeave.tom"),
    };

    hash_t future_mesh = AssetManager::load_mesh_async(project::get_asset_path(project::DirKey::MESH) / "cube.wesh");

    // * Declare entities dependencies on future resources during entity creation
    size_t cnt = 0;
    for(size_t ii = 0; ii < future_materials.size(); ++ii)
    {
        for(size_t jj = 0; jj < 2; ++jj)
        {
            std::string obj_name = "Obj #" + std::to_string(cnt++);
            EntityID ent = create_entity(obj_name);

            ComponentMesh cmesh;
            ComponentOBB cobb;

            float scale = 1.f;
            if(jj == 0)
            {
                cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));
                cobb.init(CommonGeometry::get_extent("cube_pbr"_h));
                scale = 1.5f;
            }
            else
            {
                cmesh.set_vertex_array(CommonGeometry::get_vertex_array("icosphere_pbr"_h));
                cobb.init(CommonGeometry::get_extent("icosphere_pbr"_h));
            }

            ComponentTransform3D ctransform = {
                {-4.f + float(ii) * 2.5f, 0.f, -1.f + float(jj) * 2.5f}, {0.f, 0.f, 0.f}, scale};

            registry.assign<ComponentTransform3D>(ent, ctransform);
            registry.assign<ComponentMesh>(ent, cmesh);
            registry.assign<ComponentOBB>(ent, cobb);

            AssetManager::on_material_ready(future_materials[ii], [this, ent = ent](const ComponentPBRMaterial& mat) {
                registry.assign<ComponentPBRMaterial>(ent, mat);
            });
        }
    }

    {
        std::string obj_name = "Some Object";
        EntityID ent = create_entity(obj_name);

        ComponentTransform3D ctransform = {{0.f, 0.f, 4.f}, {0.f, 0.f, 0.f}, 1.f};
        registry.assign<ComponentTransform3D>(ent, ctransform);

        AssetManager::on_mesh_ready(future_mesh, [this, ent = ent](const Mesh& mesh) {
            ComponentMesh cmesh;
            ComponentOBB cobb;
            cmesh.set_vertex_array(mesh.VAO);
            cobb.init(mesh.extent);
            registry.assign<ComponentMesh>(ent, cmesh);
            registry.assign<ComponentOBB>(ent, cobb);
        });

        AssetManager::on_material_ready(future_materials[0], [this, ent = ent](const ComponentPBRMaterial& mat) {
            registry.assign<ComponentPBRMaterial>(ent, mat);
        });
    }

    load_hdr_environment(project::get_asset_path(project::DirKey::HDR) / "small_cathedral_2k.hdr");

    // * Launch async loading operations
    AssetManager::launch_async_tasks();

    return true;
}

void Scene::on_unload()
{
    selected_entity = k_invalid_entity_id;
    directional_light = k_invalid_entity_id;
    camera = k_invalid_entity_id;
    registry.clear();
    entities.clear();
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
        registry.destroy(entity);
        removed_entities_.pop();
    }
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
    registry.assign<ComponentDescription>(entity, desc);

    entities.push_back(entity);

    DLOG("editor", 1) << "[Scene] Added entity: " << name << std::endl;
}

void Scene::select(EntityID entity) { selected_entity = entity; }

void Scene::drop_selection() { selected_entity = k_invalid_entity_id; }

void Scene::mark_for_removal(EntityID entity, uint32_t reflected_component)
{
    removed_components_.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity) { removed_entities_.push(entity); }

} // namespace editor