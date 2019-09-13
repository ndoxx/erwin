#include "render/renderer_2d.h"

#include "core/core.h"
#include "debug/logger.h"
#include "render/texture.h"

#include "platform/ogl_shader.h" // TMP
#include "render/render_device.h" // TMP: API access pushed to render thread at some point

#include <bitset>

namespace erwin
{

ShaderBank Renderer2D::shader_bank;

[[maybe_unused]] static uint32_t make_key(bool is_state_mutation,
						 				  uint32_t layer_index, 
						 				  uint32_t material_index = 0)
{
	return  (material_index << 0)
		  | (uint32_t(is_state_mutation) << 7)
		  | (layer_index << 8);
}

Renderer2D::Renderer2D()
{
	query_timer_ = QueryTimer::create();
}

Renderer2D::~Renderer2D()
{
	delete query_timer_;
}

void Renderer2D::begin_scene(uint32_t layer_index)
{
	//DLOG("render",1) << "--------" << std::endl;
	current_layer_ = layer_index; // For queue item key creation

	if(profiling_enabled_)
		query_timer_->start();
}

void Renderer2D::end_scene()
{
	// * Flush render thread

	if(profiling_enabled_)
	{
		auto render_duration = query_timer_->stop();
		// TMP: display curve in widget instead
		DLOG("render",1) << std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count() << "µs" << std::endl;
	}
}

void Renderer2D::submit(const RenderState& state)
{
	[[maybe_unused]] uint64_t key = make_key(true, current_layer_);
	//DLOG("render",1) << "sta: " << std::bitset<32>(key) << std::endl;

	// TMP: direct rendering for now, will submit to render thread then
	if(state.render_target == RenderTarget::Default)
		Gfx::device->bind_default_frame_buffer();
	else
		W_ASSERT(false, "Only default render target supported for now!");

	Gfx::device->set_cull_mode(state.rasterizer_state);

	if(state.blend_state == BlendState::Alpha)
		Gfx::device->set_std_blending();
	else
		Gfx::device->disable_blending();

	Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
	if(state.depth_stencil_state.stencil_test_enabled)
	{
		Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
		Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
	}

	Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
	if(state.depth_stencil_state.depth_test_enabled)
		Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);
}

void Renderer2D::submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params)
{
	[[maybe_unused]] uint64_t key = make_key(false, current_layer_, shader_bank.get_index(shader_name));
	//DLOG("render",1) << "cmd: " << std::bitset<32>(key) << std::endl;

	// TMP: direct rendering for now, will submit to render thread then
	const Shader& shader = shader_bank.get(shader_name);
	shader.bind();

	// * Setup uniforms
	// Setup samplers
	for(auto&& [sampler, texture]: params.texture_slots)
	{
		uint32_t slot = shader.get_texture_slot(sampler);
    	texture->bind(slot);
    	static_cast<const OGLShader&>(shader).send_uniform<int>(sampler, slot);
	}

	// * Draw
    Gfx::device->draw_indexed(va);
}



// --------------------------------------------------------------

BatchRenderer2D::BatchRenderer2D(uint32_t num_batches, uint32_t max_batch_count):
max_batch_count_(max_batch_count)
{
	BufferLayout vertex_color_layout =
	{
	    {"a_position"_h, ShaderDataType::Vec3},
	    {"a_color"_h,    ShaderDataType::Vec3},
	};

	uint32_t num_vertices = max_batch_count_ * 4; // Quads
	uint32_t num_indices = max_batch_count_ * 6;  // 2 triangles

	for(int ii=0; ii<num_batches; ++ii)
	{
		auto vb = std::shared_ptr<VertexBuffer>(VertexBuffer::create(nullptr, num_vertices, vertex_color_layout, DrawMode::Dynamic));
		auto ib = std::shared_ptr<IndexBuffer>(IndexBuffer::create(nullptr, num_indices, DrawPrimitive::Triangles, DrawMode::Dynamic));
		auto va = std::shared_ptr<VertexArray>(VertexArray::create());
		va->set_vertex_buffer(vb);
		va->set_index_buffer(ib);
		quad_batch_vas_.push_back(va);
	}
}

BatchRenderer2D::~BatchRenderer2D()
{

}

void BatchRenderer2D::begin_scene(uint32_t layer_index)
{

}

void BatchRenderer2D::end_scene()
{

}


} // namespace erwin