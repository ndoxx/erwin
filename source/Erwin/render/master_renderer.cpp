#include "render/master_renderer.h"
#include "render/render_device.h"
#include "render/query_timer.h"
#include "filesystem/filesystem.h"
#include "core/intern_string.h"
#include "debug/logger.h"

#include "imgui.h"
#include "imgui/imgui_utils.h"

#include <cstring>

namespace erwin
{

std::unique_ptr<MasterRenderer> MasterRenderer::instance_ = nullptr;

ShaderBank MasterRenderer::shader_bank;

void MasterRenderer::create()
{
	DLOGN("render") << "[MasterRenderer] Initializing." << std::endl;
	instance_ = std::make_unique<MasterRenderer>();
}

void MasterRenderer::kill()
{
	DLOGN("render") << "[MasterRenderer] Releasing." << std::endl;
	instance_ = nullptr;
}

MasterRenderer::MasterRenderer():
profiling_enabled_(false)
{
	query_timer_ = QueryTimer::create();

	shader_bank.load(filesystem::get_system_asset_dir() / "shaders/color_inst_shader.spv");
	// shader_bank.load(filesystem::get_system_asset_dir() / "shaders/color_inst_shader.glsl");
	shader_bank.load(filesystem::get_system_asset_dir() / "shaders/post_proc.spv");
	// shader_bank.load(filesystem::get_system_asset_dir() / "shaders/post_proc.glsl");

	add_queue<InstancedSpriteQueueData>(0,8192,32,
										std::bind(&MasterRenderer::execute_isp, this, std::placeholders::_1),
										std::bind(&MasterRenderer::apply_state, this, std::placeholders::_1));
	add_queue<PostProcessingQueueData>(1,32,32,
									   std::bind(&MasterRenderer::execute_pp, this, std::placeholders::_1),
									   std::bind(&MasterRenderer::apply_state, this, std::placeholders::_1));
}

MasterRenderer::~MasterRenderer()
{
	
}

void MasterRenderer::flush()
{
	reset_stats();

	if(profiling_enabled_)
		query_timer_->start();

	for(auto&& [priority, key]: queue_priority_)
	{
		queues_.at(key)->flush();
	}

	if(profiling_enabled_)
	{
		auto render_duration = query_timer_->stop();
		stats_.render_time = std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count();
	}
}

// TMP
void MasterRenderer::on_imgui_render()
{
    if(profiling_enabled_)
    {
    	static uint32_t s_frame_cnt = 0;
    	ImGui::Begin("Stats");
        	// ImGui::Text("#Batches: %d", stats_.batches);
        	// ImGui::Separator();
        	ImGui::PlotVar("Draw time (µs)", stats_.render_time, 0.0f, 7000.f);
    	
            if(++s_frame_cnt>200)
            {
                s_frame_cnt = 0;
                ImGui::PlotVarFlushOldEntries();
            }
    	ImGui::End();
    }
}

void MasterRenderer::apply_state(const PassState& state)
{
	if(prev_state_.render_target != state.render_target)
	{
		Gfx::framebuffer_pool->bind(state.render_target);
		prev_state_.render_target = state.render_target;
	}

	if(prev_state_.rasterizer_state.clear_color != state.rasterizer_state.clear_color)
	{
		Gfx::device->set_clear_color(state.rasterizer_state.clear_color.r, state.rasterizer_state.clear_color.g, state.rasterizer_state.clear_color.b, state.rasterizer_state.clear_color.a);
		prev_state_.rasterizer_state.clear_color = state.rasterizer_state.clear_color;
	}

	Gfx::device->clear(CLEAR_COLOR_FLAG);

	if(prev_state_.rasterizer_state.cull_mode != state.rasterizer_state.cull_mode)
	{
		Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);
		prev_state_.rasterizer_state.cull_mode = state.rasterizer_state.cull_mode;
	}

	if(prev_state_.blend_state != state.blend_state)
	{
		if(state.blend_state == BlendState::Alpha)
			Gfx::device->set_std_blending();
		else
			Gfx::device->disable_blending();
		prev_state_.blend_state = state.blend_state;
	}

	if(prev_state_.depth_stencil_state.stencil_test_enabled != state.depth_stencil_state.stencil_test_enabled)
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

	if(prev_state_.depth_stencil_state.depth_test_enabled != state.depth_stencil_state.depth_test_enabled)
	{
		Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
		if(state.depth_stencil_state.depth_test_enabled)
			Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);

		prev_state_.depth_stencil_state.depth_test_enabled = state.depth_stencil_state.depth_test_enabled;
		prev_state_.depth_stencil_state.depth_func = state.depth_stencil_state.depth_func;
	}
}

void MasterRenderer::execute_isp(const InstancedSpriteQueueData& data)
{
	W_ASSERT(data.VAO, "InstancedSpriteQueueData: must initialize VAO.");

	const Shader& shader = shader_bank.get("color_inst_shader"_h);
	shader.bind();

	if(data.texture && (state_cache_.texture!=data.texture->get_unique_id()))
	{
		shader.attach_texture("us_atlas"_h, *data.texture);
		state_cache_.texture = data.texture->get_unique_id();
	}

	if(data.UBO && (state_cache_.UBO!=data.UBO->get_unique_id()))
	{
		shader.attach_uniform_buffer(*data.UBO);
		state_cache_.UBO = data.UBO->get_unique_id();
	}

	if(data.SSBO && (state_cache_.SSBO!=data.SSBO->get_unique_id()))
	{
		shader.attach_shader_storage(*data.SSBO);
		state_cache_.SSBO = data.SSBO->get_unique_id();
	}

	Gfx::device->draw_indexed_instanced(*data.VAO, data.instance_count);
}

void MasterRenderer::execute_pp(const PostProcessingQueueData& data)
{
	W_ASSERT(data.VAO, "PostProcessingQueueData: must initialize VAO.");

	const Shader& shader = shader_bank.get("post_proc"_h);
	shader.bind();

	if(data.input_framebuffer)
	{
		auto&& albedo_tex = Gfx::framebuffer_pool->get_texture(data.input_framebuffer, data.framebuffer_texture_index);
		shader.attach_texture("us_input"_h, albedo_tex);
		state_cache_.texture = albedo_tex.get_unique_id();
	}

	if(data.UBO && (state_cache_.UBO!=data.UBO->get_unique_id()))
	{
		shader.attach_uniform_buffer(*data.UBO);
		state_cache_.UBO = data.UBO->get_unique_id();
	}

	Gfx::device->draw_indexed(*data.VAO);
}

} // namespace erwin