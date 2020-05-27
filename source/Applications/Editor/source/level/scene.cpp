#include "level/scene.h"
#include "imgui/font_awesome.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
#include "asset/asset_manager.h"
#include "render/renderer_3d.h"
#include "render/renderer.h"
#include "debug/logger.h"

#include <tuple>

using namespace erwin;

namespace editor
{

void Scene::load()
{

}

void Scene::unload()
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