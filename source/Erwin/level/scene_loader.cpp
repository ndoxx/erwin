#include "level/scene_loader.h"
#include "asset/asset_manager.h"
#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/light.h"
#include "imgui/font_awesome.h"
#include "level/scene.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

namespace erwin
{

void SceneLoader::load_scene_stub(const fs::path& materials_path, const fs::path& hdrs_path)
{
    // TMP -> Implement proper scene loading

    Scene::camera_controller.init(1280.f / 1024.f, 60, 0.1f, 100.f);

    // Load resources
    Scene::load_hdr_environment(hdrs_path / "small_cathedral_2k.hdr");
    ComponentPBRMaterial mat_greasy_metal = AssetManager::load_PBR_material(materials_path / "greasyMetal.tom");

    Material mat_sun;
    mat_sun.archetype = "Sun"_h;
    mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
    mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
    mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
    Renderer3D::register_shader(mat_sun.shader);
    Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

    {
        EntityID ent = Scene::registry.create();

        ComponentDirectionalLight cdirlight;
        cdirlight.set_position(47.626f, 49.027f);
        cdirlight.color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_strength = 0.1f;
        cdirlight.brightness = 3.7f;

        ComponentDirectionalLightMaterial renderable;
        renderable.set_material(mat_sun);
        renderable.material_data.scale = 0.2f;

        Scene::registry.assign<ComponentDirectionalLight>(ent, cdirlight);
        Scene::registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

        Scene::directional_light = ent;
        Scene::add_entity(ent, "Sun", W_ICON(SUN_O));
    }

    {
        EntityID ent = Scene::registry.create();

        ComponentTransform3D ctransform = {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 1.8f};

        ComponentOBB cOBB(CommonGeometry::get_extent("cube_pbr"_h));
        cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

        ComponentMesh cmesh;
        cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

        Scene::registry.assign<ComponentTransform3D>(ent, ctransform);
        Scene::registry.assign<ComponentOBB>(ent, cOBB);
        Scene::registry.assign<ComponentMesh>(ent, cmesh);
        Scene::registry.assign<ComponentPBRMaterial>(ent, mat_greasy_metal);

        Scene::add_entity(ent, "Cube #0");
    }

    Scene::camera_controller.set_position({-5.8f, 2.3f, -5.8f});
    Scene::camera_controller.set_angles(228.f, 5.f);
}

void SceneLoader::clear_scene()
{
    Scene::selected_entity = k_invalid_entity_id;
    Scene::directional_light = k_invalid_entity_id;
    Renderer::destroy(Scene::environment.environment_map);
    Renderer::destroy(Scene::environment.diffuse_irradiance_map);
    Renderer::destroy(Scene::environment.prefiltered_env_map);
    Scene::environment.environment_map = {};
    Scene::environment.diffuse_irradiance_map = {};
    Scene::environment.prefiltered_env_map = {};
    Scene::registry.clear();
    Scene::entities.clear();
}

} // namespace erwin