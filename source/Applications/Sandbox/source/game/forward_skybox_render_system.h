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
	ShaderHandle skybox_shader;

public:
	void init()
	{
		static constexpr uint32_t cm_width = 512;
		static constexpr uint32_t cm_height = 512;

		// Proceduraly generate a cubemap
		// cubemap face order: (-x, +x, -y, +y, -z, +z)
		static std::array<std::array<uint8_t,3*cm_width*cm_height>, 6> faces;

		for(int ff=0; ff<6; ++ff)
		{
			for(int yy=0; yy<cm_height; ++yy)
			{
				float h = yy/float(cm_height-1);
				h = h*h;
				for(int xx=0; xx<cm_width; ++xx)
				{
					uint32_t index = yy*cm_width + xx;
					if(ff!=2 && ff!=3)
					{
						faces[ff][3*index+0] = uint8_t((1-h)*51 +h*128);
						faces[ff][3*index+1] = uint8_t((1-h)*153+h*128);
						faces[ff][3*index+2] = uint8_t((1-h)*255+h*128);
					}
					else if(ff==2)
					{
						faces[ff][3*index+0] = uint8_t(51);
						faces[ff][3*index+1] = uint8_t(153);
						faces[ff][3*index+2] = uint8_t(255);
					}
					else
					{
						faces[ff][3*index+0] = uint8_t(128);
						faces[ff][3*index+1] = uint8_t(128);
						faces[ff][3*index+2] = uint8_t(128);
					}
				}
			}
		}

		CubemapDescriptor desc
		{
		    cm_width,
		    cm_height,
		    {faces[0].data(),faces[1].data(),faces[2].data(),faces[3].data(),faces[4].data(),faces[5].data()},
		    ImageFormat::RGB8,
		    MIN_LINEAR | MAG_LINEAR,
		    TextureWrap::CLAMP_TO_EDGE,
		    false
		};

		cubemap = Renderer::create_cubemap(desc);
	}

	void render()
	{
		Renderer3D::draw_skybox(cubemap);
	}
};


} // namespace erwin