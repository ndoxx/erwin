#include "level/scene.h"
#include "imgui/font_awesome.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
#include "asset/asset_manager.h"
#include "render/renderer_3d.h"
#include "render/renderer.h"
#include "debug/logger.h"

#include <queue>
#include <tuple>

namespace erwin
{

EntityID Scene::selected_entity = k_invalid_entity_id;
EntityID Scene::directional_light = k_invalid_entity_id;
std::vector<EntityID> Scene::entities;
FreeflyController Scene::camera_controller;
entt::registry Scene::registry;

static std::queue<std::tuple<EntityID, uint32_t>> s_removed_components;
static std::queue<EntityID> s_removed_entities;

Scene::Environment Scene::environment;

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
	s_removed_components.push({entity, reflected_component});
}

void Scene::mark_for_removal(EntityID entity)
{
	s_removed_entities.push(entity);
}

void Scene::cleanup()
{
	// Removed marked components
	while(!s_removed_components.empty())
	{
		auto&& [entity, reflected_component] = s_removed_components.front();
		invoke(W_METAFUNC_REMOVE_COMPONENT, reflected_component, Scene::registry, entity);
		s_removed_components.pop();
	}

	// Removed marked entities and all their components
	while(!s_removed_entities.empty())
	{
		auto entity = s_removed_entities.front();
		registry.destroy(entity);
		s_removed_entities.pop();
	}
}


} // namespace erwin