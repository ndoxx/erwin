#pragma once

#include "erwin.h"
#include "level/scene.h"

#include <array>

namespace erwin
{

class ForwardSkyboxRenderSystem
{
private:
	CubemapHandle env_map;
	CubemapHandle diffuse_irradiance_map;
	CubemapHandle prefiltered_env_map;

public:
	void init()
	{
		Texture2DDescriptor hdr_desc;
		// TextureHandle hdr_tex = AssetManager::load_image("textures/hdr/autumn_park_2k.hdr", hdr_desc);
		// TextureHandle hdr_tex = AssetManager::load_image("textures/hdr/lakeside_1k.hdr", hdr_desc);
		// TextureHandle hdr_tex = AssetManager::load_image("textures/hdr/dirt_bike_track_01_1k.hdr", hdr_desc);
		// TextureHandle hdr_tex = AssetManager::load_image("textures/hdr/georgentor_2k.hdr", hdr_desc);
		TextureHandle hdr_tex = AssetManager::load_image("textures/hdr/small_cathedral_2k.hdr", hdr_desc);
		env_map = Renderer3D::generate_cubemap_hdr(hdr_tex, hdr_desc.height);
		Renderer::destroy(hdr_tex);
		diffuse_irradiance_map = Renderer3D::generate_irradiance_map(env_map);
		prefiltered_env_map = Renderer3D::generate_prefiltered_map(env_map, hdr_desc.height);
		Renderer3D::set_environment(diffuse_irradiance_map, prefiltered_env_map);
	}

	void render()
	{
		Renderer3D::draw_skybox(env_map);
		// Renderer3D::draw_skybox(diffuse_irradiance_map);
		// Renderer3D::draw_skybox(prefiltered_env_map);
	}
};


} // namespace erwin