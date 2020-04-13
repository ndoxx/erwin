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
		skybox_shader = AssetManager::load_system_shader("shaders/skybox.glsl");
		Renderer3D::register_shader(skybox_shader, {}, ShaderFlags::SAMPLE_ENVIRONMENT);
		// Renderer3D::set_environment_cubemap(cubemap);
	}

	void render()
	{
		VertexArrayHandle cube = CommonGeometry::get_vertex_array("cube"_h);

		RenderState state;
		state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
		state.rasterizer_state.cull_mode = CullMode::Front;
		state.blend_state = BlendState::Opaque;
		state.depth_stencil_state.depth_func = DepthFunc::LEqual;
		state.depth_stencil_state.depth_test_enabled = true;
		state.depth_stencil_state.depth_lock = true;
		auto state_flags = state.encode();

		SortKey key;
		key.set_sequence(0, Renderer::next_layer_id(), skybox_shader);

		DrawCall dc(DrawCall::Indexed, state_flags, skybox_shader, cube);
		dc.set_cubemap(cubemap, 0);
		Renderer::submit(key.encode(), dc);

		// Renderer3D::begin_forward_pass();
		// Renderer3D::draw_mesh(cube, glm::mat4(1.f), skybox_shader);
		// Renderer3D::end_forward_pass();
	}
};


} // namespace erwin