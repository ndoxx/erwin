#pragma once

#include "erwin.h"
#include "level/scene.h"
#include "game/game_components.h"

#include <array>

namespace erwin
{

class ForwardSkyboxRenderSystem
{
private:
	CubemapHandle env_map;
	CubemapHandle diffuse_irradiance_map;

public:
	void init()
	{
		env_map = AssetManager::load_cubemap_hdr("textures/hdr/autumn_park_2k.hdr");
		// env_map = AssetManager::load_cubemap_hdr("textures/hdr/lakeside_1k.hdr");
		// env_map = AssetManager::load_cubemap_hdr("textures/hdr/dirt_bike_track_01_1k.hdr");
		diffuse_irradiance_map = Renderer3D::generate_irradiance_map(env_map);
	}

	void render()
	{
		// Renderer3D::draw_skybox(env_map);
		Renderer3D::draw_skybox(diffuse_irradiance_map);
	}
};


} // namespace erwin