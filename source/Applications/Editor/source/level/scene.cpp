#include "level/scene.h"
#include "imgui/font_awesome.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
#include "asset/asset_manager.h"
#include "render/renderer_3d.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "debug/logger.h"

#include "project/project.h"
#include "entity/component_PBR_material.h"
#include "entity/component_bounding_box.h"
#include "entity/component_dirlight_material.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/component_camera.h"
#include "entity/light.h"

#include <tuple>

using namespace erwin;

namespace editor
{

bool Scene::on_load()
{
    // TMP stub -> Implement proper scene loading

    // Load resources
    load_hdr_environment(project::get_asset_path(project::DirKey::HDR) / "small_cathedral_2k.hdr");
    ComponentPBRMaterial mat_greasy_metal = AssetManager::load_PBR_material(project::get_asset_path(project::DirKey::MATERIAL) / "greasyMetal.tom");

    Material mat_sun;
    mat_sun.archetype = "Sun"_h;
    mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
    mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
    mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
    Renderer3D::register_shader(mat_sun.shader);
    Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

    {
        EntityID ent = registry.create();

        ComponentTransform3D transform({-5.8f, 2.3f, -5.8f}, {5.f, 228.f, 0.f}, 1.f);

        registry.assign<ComponentCamera3D>(ent);
        registry.assign<ComponentTransform3D>(ent, transform);
        camera = ent;
        add_entity(ent, "Camera", W_ICON(VIDEO_CAMERA));
    }

    {
        EntityID ent = registry.create();

        ComponentDirectionalLight cdirlight;
        cdirlight.set_position(47.626f, 49.027f);
        cdirlight.color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_color = {0.95f, 0.85f, 0.5f};
        cdirlight.ambient_strength = 0.1f;
        cdirlight.brightness = 3.7f;

        ComponentDirectionalLightMaterial renderable;
        renderable.set_material(mat_sun);
        renderable.material_data.scale = 0.2f;

        registry.assign<ComponentDirectionalLight>(ent, cdirlight);
        registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

        directional_light = ent;
        add_entity(ent, "Sun", W_ICON(SUN_O));
    }

    {
        EntityID ent = registry.create();

        ComponentTransform3D ctransform = {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, 1.8f};

        ComponentOBB cOBB(CommonGeometry::get_extent("cube_pbr"_h));
        cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

        ComponentMesh cmesh;
        cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

        registry.assign<ComponentTransform3D>(ent, ctransform);
        registry.assign<ComponentOBB>(ent, cOBB);
        registry.assign<ComponentMesh>(ent, cmesh);
        registry.assign<ComponentPBRMaterial>(ent, mat_greasy_metal);

        add_entity(ent, "Cube #0");
    }

	return true;
}

void Scene::on_unload()
{
    selected_entity = k_invalid_entity_id;
    directional_light = k_invalid_entity_id;
    camera = k_invalid_entity_id;
    Renderer::destroy(environment.environment_map);
    Renderer::destroy(environment.diffuse_irradiance_map);
    Renderer::destroy(environment.prefiltered_env_map);
    environment.environment_map = {};
    environment.diffuse_irradiance_map = {};
    environment.prefiltered_env_map = {};
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
	Texture2DDescriptor hdr_desc;
	TextureHandle hdr_tex = AssetManager::load_image(hdr_file, hdr_desc);
	environment.environment_map = Renderer3D::generate_cubemap_hdr(hdr_tex, hdr_desc.height);
	Renderer::destroy(hdr_tex);
	environment.diffuse_irradiance_map = Renderer3D::generate_irradiance_map(environment.environment_map);
	environment.prefiltered_env_map = Renderer3D::generate_prefiltered_map(environment.environment_map, hdr_desc.height);
	Renderer3D::set_environment(environment.diffuse_irradiance_map, environment.prefiltered_env_map);
}

void Scene::add_entity(EntityID entity, const std::string& name, const char* _icon)
{
	ComponentDescription desc = {name, (_icon) ? _icon : W_ICON(CUBE), ""};
	registry.assign<ComponentDescription>(entity, desc);

	entities.push_back(entity);

	DLOG("editor",1) << "[Scene] Added entity: " << name << std::endl;
}

void Scene::select(EntityID entity)
{
	selected_entity = entity;
}

void Scene::drop_selection()
{
	selected_entity = k_invalid_entity_id;
}

void Scene::mark_for_removal(EntityID entity, uint32_t reflected_component)
{
	removed_components_.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity)
{
	removed_entities_.push(entity);
}



} // namespace editor