#include "render/renderer_2d.h"

#include "core/core.h"
#include "debug/logger.h"
#include "render/texture.h"

#include "platform/ogl_shader.h" // TMP
#include "render/render_device.h" // TMP: API access pushed to render thread at some point

#include <bitset>
#include <iostream>

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

ShaderBank BatchRenderer2D::shader_bank;

BatchRenderer2D::BatchRenderer2D(uint32_t num_batches, uint32_t max_batch_count):
max_batch_count_(max_batch_count),
max_batches_(num_batches)
{
	BufferLayout vertex_color_layout =
	{
	    {"a_position"_h, ShaderDataType::Vec3},
	    {"a_color"_h,    ShaderDataType::Vec3},
	};

	uint32_t num_vertices = max_batch_count_ * 4; // Quads
	uint32_t num_indices = max_batch_count_ * 6;  // 2 triangles

	for(int ii=0; ii<max_batches_; ++ii)
	{
		auto vb = std::shared_ptr<VertexBuffer>(VertexBuffer::create(nullptr, num_vertices, vertex_color_layout, DrawMode::Dynamic));
		auto ib = std::shared_ptr<IndexBuffer>(IndexBuffer::create(nullptr, num_indices, DrawPrimitive::Triangles, DrawMode::Dynamic));
		auto va = std::shared_ptr<VertexArray>(VertexArray::create());
		va->set_vertex_buffer(vb);
		va->set_index_buffer(ib);
		quad_batch_vas_.push_back(va);
	}

	// Load shader
	shader_bank.load("shaders/color_shader.glsl");

	query_timer_ = QueryTimer::create();
}

BatchRenderer2D::~BatchRenderer2D()
{
	delete query_timer_;
}

void BatchRenderer2D::begin_scene(uint32_t layer_index)
{
	// Reset
	current_batch_ = 0;
	current_batch_count_ = 0;
	current_batch_v_offset_ = 0;
	current_batch_i_offset_ = 0;

	if(profiling_enabled_)
		query_timer_->start();
}

void BatchRenderer2D::end_scene()
{
	// Draw last batch
	const Shader& shader = shader_bank.get("color_shader"_h);
	shader.bind();
    Gfx::device->draw_indexed(quad_batch_vas_[current_batch_], current_batch_count_*6);
	shader.unbind();

	if(profiling_enabled_)
	{
		auto render_duration = query_timer_->stop();
		// TMP: display curve in widget instead
		DLOG("render",1) << std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count() << "µs" << std::endl;
	}
}

void BatchRenderer2D::submit(const RenderState& state)
{
	//[[maybe_unused]] uint64_t key = make_key(true, current_layer_);
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

void BatchRenderer2D::draw_quad(const math::vec2& position, 
				   			    const math::vec2& scale,
				   			    const math::vec3& color)
{
	// * Select a batch vertex array
	// Check that current batch has enough space, if not, draw current and start to fill next batch
	if(current_batch_count_ == max_batch_count_)
	{
		const Shader& shader = shader_bank.get("color_shader"_h);
		shader.bind();
    	Gfx::device->draw_indexed(quad_batch_vas_[current_batch_], current_batch_count_*6);
		shader.unbind();

		++current_batch_;
		W_ASSERT(current_batch_<=max_batches_, "[BatchRenderer2D] Batch index exceeds max batch number.");
		current_batch_count_    = 0;
		current_batch_v_offset_ = 0;
		current_batch_i_offset_ = 0;
	}

	// * Push vertices and indices to current batch
	auto& vb = quad_batch_vas_[current_batch_]->get_vertex_buffer();
	auto& ib = quad_batch_vas_[current_batch_]->get_index_buffer();

	static const int VERT_FLOAT_COUNT = 24;
	static const int IND_UINT_COUNT = 6;

	float vdata[VERT_FLOAT_COUNT] = 
	{
		position.x()-0.5f*scale.x(), position.y()-0.5f*scale.y(), 0.0f,   color.r(), color.g(), color.b(),
		position.x()+0.5f*scale.x(), position.y()-0.5f*scale.y(), 0.0f,   color.r(), color.g(), color.b(),
		position.x()+0.5f*scale.x(), position.y()+0.5f*scale.y(), 0.0f,   color.r(), color.g(), color.b(),
		position.x()-0.5f*scale.x(), position.y()+0.5f*scale.y(), 0.0f,   color.r(), color.g(), color.b(),
	};

	uint32_t v_offset = current_batch_count_ * 4;
	uint32_t idata[IND_UINT_COUNT] = 
	{
		v_offset + 0, 
		v_offset + 1, 
		v_offset + 2, 
		v_offset + 2, 
		v_offset + 3, 
		v_offset + 0
	};

	quad_batch_vas_[current_batch_]->bind();
	vb.stream(vdata, VERT_FLOAT_COUNT, current_batch_v_offset_);
	ib.stream(idata, IND_UINT_COUNT, current_batch_i_offset_);
	quad_batch_vas_[current_batch_]->unbind();

	// Update batch variables
	current_batch_v_offset_ += VERT_FLOAT_COUNT;
	current_batch_i_offset_ += IND_UINT_COUNT;
	++current_batch_count_;

	W_ASSERT(Gfx::device->get_error()==0, "Driver error!");
}


} // namespace erwin