#include "render/master_renderer.h"
#include "render/render_device.h"
#include "filesystem/filesystem.h"
#include "debug/logger.h"
#include <cstring>

namespace erwin
{

ShaderBank MasterRenderer::shader_bank;

void MasterRenderer::create()
{
	DLOGN("render") << "[MasterRenderer] Initializing." << std::endl;
	MASTER_RENDERER = std::make_unique<MasterRenderer>();
}

void MasterRenderer::kill()
{
	DLOGN("render") << "[MasterRenderer] Releasing." << std::endl;
	MASTER_RENDERER = nullptr;
}

MasterRenderer::MasterRenderer()
{
	add_queue<InstancedSpriteQueueData>(0,8192,32,
										std::bind(&MasterRenderer::execute, this, std::placeholders::_1),
										std::bind(&MasterRenderer::apply_state, this, std::placeholders::_1));

	shader_bank.load(filesystem::get_system_asset_dir() / "shaders/color_inst_shader.spv");
}

MasterRenderer::~MasterRenderer()
{
	
}

void MasterRenderer::flush()
{
	for(auto&& [priority, key]: queue_priority_)
	{
		queues_.at(key)->flush();
	}
}

void MasterRenderer::apply_state(const PassState& state)
{
	if(state.render_target != prev_state_.render_target)
	{
		if(state.render_target)
			Gfx::framebuffer_pool->bind(state.render_target);
		else
			Gfx::device->bind_default_frame_buffer();
		prev_state_.render_target = state.render_target;
	}

	if(memcmp(&state.rasterizer_state.clear_color, &prev_state_.rasterizer_state.clear_color, sizeof(glm::vec4)))
	{
		Gfx::device->set_clear_color(state.rasterizer_state.clear_color.r, state.rasterizer_state.clear_color.g, state.rasterizer_state.clear_color.b, state.rasterizer_state.clear_color.a);
		prev_state_.rasterizer_state.clear_color = state.rasterizer_state.clear_color;
	}
	Gfx::device->clear(CLEAR_COLOR_FLAG);

	if(state.rasterizer_state.cull_mode != prev_state_.rasterizer_state.cull_mode)
	{
		Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);
		prev_state_.rasterizer_state.cull_mode = state.rasterizer_state.cull_mode;
	}

	if(state.blend_state != prev_state_.blend_state)
	{
		if(state.blend_state == BlendState::Alpha)
			Gfx::device->set_std_blending();
		else
			Gfx::device->disable_blending();
		prev_state_.blend_state = state.blend_state;
	}

	if(state.depth_stencil_state.stencil_test_enabled != prev_state_.depth_stencil_state.stencil_test_enabled)
	{
		Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
		if(state.depth_stencil_state.stencil_test_enabled)
		{
			Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
			Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
		}
		prev_state_.depth_stencil_state.stencil_test_enabled = state.depth_stencil_state.stencil_test_enabled;
		prev_state_.depth_stencil_state.stencil_operator = state.depth_stencil_state.stencil_operator;
		prev_state_.depth_stencil_state.stencil_func = state.depth_stencil_state.stencil_func;
	}

	if(state.depth_stencil_state.depth_test_enabled != prev_state_.depth_stencil_state.depth_test_enabled)
	{
		Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
		if(state.depth_stencil_state.depth_test_enabled)
			Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);

		prev_state_.depth_stencil_state.depth_test_enabled = state.depth_stencil_state.depth_test_enabled;
		prev_state_.depth_stencil_state.depth_func = state.depth_stencil_state.depth_func;
	}
}

void MasterRenderer::execute(const InstancedSpriteQueueData& data)
{
	static W_ID s_last_tex_id  = 0,
			    s_last_UBO_id  = 0,
			    s_last_SSBO_id = 0,
			    s_last_VAO_id  = 0;

	W_ASSERT(data.VAO, "InstancedSpriteQueueData: must initialize VAO.");

	const Shader& shader = shader_bank.get("color_inst_shader"_h);
	shader.bind();

	if(data.texture && (data.texture->get_unique_id() != s_last_tex_id))
	{
		shader.attach_texture("us_atlas"_h, *data.texture);
		s_last_tex_id = data.texture->get_unique_id();
	}

	if(data.UBO && (data.UBO->get_unique_id() != s_last_UBO_id))
	{
		shader.attach_uniform_buffer(*data.UBO);
		s_last_UBO_id = data.UBO->get_unique_id();
	}

	if(data.SSBO && (data.SSBO->get_unique_id() != s_last_SSBO_id))
	{
		shader.attach_shader_storage(*data.SSBO);
		s_last_SSBO_id = data.SSBO->get_unique_id();
	}

	if(data.VAO->get_unique_id() != s_last_VAO_id)
	{
		Gfx::device->draw_indexed_instanced(*data.VAO, data.instance_count);
		s_last_VAO_id = data.VAO->get_unique_id();
	}
}

} // namespace erwin