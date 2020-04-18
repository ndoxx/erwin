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
	CubemapHandle cubemap;

public:
	void init()
	{
		cubemap = AssetManager::load_cubemap_hdr("textures/hdr/autumn_park_2k.hdr");
	}

	void render()
	{
		Renderer3D::draw_skybox(cubemap);
	}
};


} // namespace erwin