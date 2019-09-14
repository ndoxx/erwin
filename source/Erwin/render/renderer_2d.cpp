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

ShaderBank BatchRenderer2D::shader_bank;

[[maybe_unused]] static uint32_t make_key(bool is_state_mutation,
						 				  uint32_t layer_index, 
						 				  uint32_t material_index = 0)
{
	return  (material_index << 0)
		  | (uint32_t(is_state_mutation) << 7)
		  | (layer_index << 8);
}

static const BufferLayout s_vertex_color_layout =
{
    {"a_position"_h, ShaderDataType::Vec3},
    {"a_color"_h,    ShaderDataType::Vec3},
};

static const int VERT_FLOAT_COUNT = 3*6; // Num vertex float component per quad (4 vertex * 6 components)

BatchRenderer2D::BatchRenderer2D(uint32_t num_batches, uint32_t max_batch_count):
max_batch_count_(max_batch_count)
{
	vertex_list_.reserve(max_batch_count * VERT_FLOAT_COUNT);

	for(int ii=0; ii<num_batches; ++ii)
		create_batch(s_vertex_color_layout);

	// Load shader
	shader_bank.load("shaders/color_dup_shader.glsl");

	query_timer_ = QueryTimer::create();
}

BatchRenderer2D::~BatchRenderer2D()
{
	delete query_timer_;
}

void BatchRenderer2D::create_batch(const BufferLayout& layout)
{
	DLOGN("render") << "[BatchRenderer2D] Generating new batch." << std::endl;
	
	uint32_t num_vertices = max_batch_count_ * 3; // 3 vertices per triangle
	//uint32_t num_indices  = max_batch_count_ * 3; // 3 indices per triangle

	auto vb = std::shared_ptr<VertexBuffer>(VertexBuffer::create(nullptr, num_vertices, layout, DrawMode::Stream));
	//auto ib = std::shared_ptr<IndexBuffer>(IndexBuffer::create(nullptr, num_indices, DrawPrimitive::Triangles, DrawMode::Stream));
	auto va = std::shared_ptr<VertexArray>(VertexArray::create());
	va->set_vertex_buffer(vb);
	//va->set_index_buffer(ib);
	quad_batch_vas_.push_back(va);

	DLOG("render",1) << "New batch size is: " << quad_batch_vas_.size() << std::endl;
}

void BatchRenderer2D::flush()
{
	// Map vertex / index lists to GPU buffers
	quad_batch_vas_[current_batch_]->bind();
	auto& vb = quad_batch_vas_[current_batch_]->get_vertex_buffer();
	vb.map(vertex_list_.data(), vertex_list_.size());

	// Clear lists for next batches
	vertex_list_.clear();
	//index_list_.clear();
}

void BatchRenderer2D::begin_scene(uint32_t layer_index)
{
	// Reset
	current_batch_ = 0;
}

void BatchRenderer2D::end_scene()
{
	// Flush last batch
	uint32_t current_batch_count = vertex_list_.size() / VERT_FLOAT_COUNT;
	flush();

	// * DRAW
	if(profiling_enabled_)
		query_timer_->start();

	const Shader& shader = shader_bank.get("color_dup_shader"_h);
	shader.bind();

	// Draw all full batches plus the last one if not empty
	for(int ii=0; ii<current_batch_; ++ii)
	{
    	// Gfx::device->draw_indexed(quad_batch_vas_[ii], max_batch_count_*3);
    	Gfx::device->draw_array(quad_batch_vas_[ii], DrawPrimitive::Triangles, max_batch_count_*3);
	}
    if(current_batch_count)
    {
    	//Gfx::device->draw_indexed(quad_batch_vas_[current_batch_], current_batch_count*3);
    	Gfx::device->draw_array(quad_batch_vas_[current_batch_], DrawPrimitive::Triangles, current_batch_count*3);
    }
	
	shader.unbind();

	if(profiling_enabled_)
	{
		auto render_duration = query_timer_->stop();
		last_render_time_ = std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count();
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

// DO NOT USE
void BatchRenderer2D::submit(std::shared_ptr<VertexArray> va, hash_t shader_name, const ShaderParameters& params)
{
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

void BatchRenderer2D::draw_quad(const glm::vec2& position, 
				   			    const glm::vec2& scale,
				   			    const glm::vec3& color)
{
	// * Select a batch vertex array
	// Check that current batch has enough space, if not, draw current and start to fill next batch
	uint32_t current_batch_count = vertex_list_.size() / VERT_FLOAT_COUNT;
	// uint32_t current_batch_count = index_list_.size() / 3;
	if(current_batch_count == max_batch_count_)
	{
		flush();

		// If current batch is the last one in list, add new batch
		if(current_batch_ == quad_batch_vas_.size()-1)
			create_batch(s_vertex_color_layout);

		++current_batch_;
		current_batch_count = 0;
	}

	// * Push vertices and indices to current batch
	// We only need to store the lower right triangle as we can regenerate the other one in the geometry shader
	float vdata[VERT_FLOAT_COUNT] = 
	{
		position.x-0.5f*scale.x, position.y-0.5f*scale.y, 0.0f,   color.r, color.g, color.b,
		position.x+0.5f*scale.x, position.y-0.5f*scale.y, 0.0f,   color.r, color.g, color.b,
		position.x+0.5f*scale.x, position.y+0.5f*scale.y, 0.0f,   color.r, color.g, color.b,
	};

	vertex_list_.insert(vertex_list_.end(), vdata, vdata + VERT_FLOAT_COUNT);

	W_ASSERT(Gfx::device->get_error()==0, "Driver error!");
}


} // namespace erwin