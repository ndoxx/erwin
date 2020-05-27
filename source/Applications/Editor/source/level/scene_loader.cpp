#include "level/scene_loader.h"
#include "asset/asset_manager.h"
#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"
#include "imgui/font_awesome.h"
#include "level/scene.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

using namespace erwin;

namespace editor
{

void SceneLoader::load_scene_stub(const fs::path& materials_path, const fs::path& hdrs_path)
{
    // TMP -> Implement proper scene loading

    auto& scene = SceneManager::create_scene<Scene>("main_scene"_h);

    // Load resources
    scene.load_hdr_environment(hdrs_path / "small_cathedral_2k.hdr");
    ComponentPBRMaterial mat_greasy_metal = AssetManager::load_PBR_material(materials_path / "greasyMetal.tom");

    Material mat_sun;
    mat_sun.archetype = "Sun"_h;
    mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
    mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
    mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
    Renderer3D::register_shader(mat_sun.shader);
    Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

    {
        EntityID ent = scene.registry.create();

        ComponentTransform3D transform({-5.8f, 2.3f, -5.8f}, {5.f, 228.f, 0.f}, 1.f);

        scene.registry.assign<ComponentCamera3D>(ent);
        scene.registry.assign<ComponentTransform3D>(ent, transform);
        scene.camera = ent;
        scene.add_entity(ent, "Camera", W_ICON(VIDEO_CAMERA));
    }

    {
        EntityID ent = scene.registry.create();

        ComponentDirectionalLight cdirlight;
        cdirlight.set_position(47.626f, 49.027f);
        cdirlight.color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_strength = 0.1f;
        cdirlight.brightness = 3.7f;

        ComponentDirectionalLightMaterial renderable;
        renderable.set_material(mat_sun);
        renderable.material_data.scale = 0.2f;

        scene.registry.assign<ComponentDirectionalLight>(ent, cdirlight);
        scene.registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

        scene.directional_light = ent;
        scene.add_entity(ent, "Sun", W_ICON(SUN_O));
    }

    {
        EntityID ent = scene.registry.create();

        ComponentTransform3D ctransform = {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 1.8f};

        ComponentOBB cOBB(CommonGeometry::get_extent("cube_pbr"_h));
        cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

        ComponentMesh cmesh;
        cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

        scene.registry.assign<ComponentTransform3D>(ent, ctransform);
        scene.registry.assign<ComponentOBB>(ent, cOBB);
        scene.registry.assign<ComponentMesh>(ent, cmesh);
        scene.registry.assign<ComponentPBRMaterial>(ent, mat_greasy_metal);

        scene.add_entity(ent, "Cube #0");
    }

    SceneManager::load_scene("main_scene"_h);
    SceneManager::make_current("main_scene"_h);
}

void SceneLoader::clear_scene()
{
    auto& scene = scn::current<Scene>();
    scene.selected_entity = k_invalid_entity_id;
    scene.directional_light = k_invalid_entity_id;
    Renderer::destroy(scene.environment.environment_map);
    Renderer::destroy(scene.environment.diffuse_irradiance_map);
    Renderer::destroy(scene.environment.prefiltered_env_map);
    scene.environment.environment_map = {};
    scene.environment.diffuse_irradiance_map = {};
    scene.environment.prefiltered_env_map = {};
    scene.registry.clear();
    scene.entities.clear();
}

} // namespace editor