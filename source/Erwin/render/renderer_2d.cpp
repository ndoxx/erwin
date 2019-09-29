#include "render/renderer_2d.h"

#include "core/core.h"
#include "debug/logger.h"
#include "render/texture.h"

#include "platform/ogl_buffer.h" // TMP
#include "platform/ogl_shader.h" // TMP
#include "render/render_device.h" // TMP: API access pushed to render thread at some point

#include <bitset>
#include <iostream>

namespace erwin
{

static const uint32_t s_num_batches_init = 1;
static const uint32_t s_max_batches = 32;
static const uint16_t s_max_batch_ttl = 60*2;

ShaderBank Renderer2D::shader_bank;

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
    {"a_uv"_h,       ShaderDataType::Vec2},
};

static const int VERT_FLOAT_COUNT = 3*5; // Num vertex float component per quad (4 vertex * 6 components)



// TMP: MOVE this to proper collision trait class?
static bool frustum_cull(const glm::vec2& position, const glm::vec2& scale, const FrustumSides& fs)
{
	// Compute each point in world space
	glm::vec3 points[4] =
	{
		glm::vec3(position.x-0.5f*scale.x, position.y-0.5f*scale.y, 1.0f),
		glm::vec3(position.x+0.5f*scale.x, position.y-0.5f*scale.y, 1.0f),
		glm::vec3(position.x+0.5f*scale.x, position.y+0.5f*scale.y, 1.0f),
		glm::vec3(position.x-0.5f*scale.x, position.y+0.5f*scale.y, 1.0f)
	};

    // For each frustum side
    for(uint32_t ii=0; ii<4; ++ii)
    {
        // Quad is considered outside iif all its vertices are above the SAME side
        bool all_out = true;
        for(const auto& p: points)
        {
            // Check if point is above side
            if(glm::dot(fs.side[ii],p)>0)
            {
                all_out = false;
                break;
            }
        }
        if(all_out)
            return true;
    }

	return false;
}

Renderer2D::Renderer2D(uint32_t max_batch_count):
max_batch_count_(max_batch_count),
current_batch_(0),
current_batch_count_(0),
batch_ttl_(s_max_batches, 0)
{
	query_timer_ = QueryTimer::create();

	if(!Gfx::framebuffer_pool->exists("fb_2d_raw"_h))
	{
		FrameBufferLayout layout =
		{
			{"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
		};
		Gfx::framebuffer_pool->create_framebuffer("fb_2d_raw"_h, make_scope<FbRatioConstraint>(), layout, false);
	}

	shader_bank.load("shaders/post_proc.glsl");

	// Create vertex array with a quad
	BufferLayout vertex_tex_layout =
	{
	    {"a_position"_h, ShaderDataType::Vec3},
	    {"a_uv"_h,       ShaderDataType::Vec2},
	};
	float sq_vdata[20] = 
	{
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	uint32_t sq_idata[6] =
	{
		0, 1, 2,   2, 3, 0
	};
	auto quad_vb = VertexBuffer::create(sq_vdata, 20, vertex_tex_layout);
	auto quad_ib = IndexBuffer::create(sq_idata, 6, DrawPrimitive::Triangles);
	screen_va_ = VertexArray::create();
	screen_va_->set_index_buffer(quad_ib);
	screen_va_->set_vertex_buffer(quad_vb);

	// UBO for post processing data
	pp_ubo_ = UniformBuffer::create("post_proc_layout", nullptr, sizeof(PostProcData), DrawMode::Dynamic);
}

Renderer2D::~Renderer2D()
{

}

void Renderer2D::begin_scene(const OrthographicCamera2D& camera, WRef<Texture2D> texture, const PostProcData& pp_data)
{
	// Set scene data
	scene_data_.view_projection_matrix = camera.get_view_projection_matrix();
	scene_data_.view_matrix = camera.get_view_matrix();
	scene_data_.frustum_sides = camera.get_frustum_sides();
	scene_data_.texture = texture;

	// Set post processing data
	post_proc_data_ = pp_data;
	post_proc_data_.fb_size = {Gfx::framebuffer_pool->get("fb_2d_raw"_h).get_width(),
				  	   		   Gfx::framebuffer_pool->get("fb_2d_raw"_h).get_height()};

	// Reset
	current_batch_ = 0;
	current_batch_count_ = 0;
	reset_stats();
}

void Renderer2D::end_scene()
{
	upload_batch();

	if(profiling_enabled_)
		query_timer_->start();

	// Render on offscreen framebuffer
	Gfx::framebuffer_pool->bind("fb_2d_raw"_h);
	Gfx::device->clear(CLEAR_COLOR_FLAG);
	flush();
	// Render generated texture on screen after post-processing
	Gfx::framebuffer_pool->bind(0);
	const Shader& post_proc_shader = Renderer2D::shader_bank.get("post_proc"_h);
	post_proc_shader.bind();
	auto&& albedo_tex = Gfx::framebuffer_pool->get_named_texture("fb_2d_raw"_h, "albedo"_h);
	post_proc_shader.attach_texture("us_input"_h, albedo_tex);
	pp_ubo_->map(&post_proc_data_);
	post_proc_shader.attach_uniform_buffer(*pp_ubo_, 0);
    Gfx::device->draw_indexed(screen_va_);
	post_proc_shader.unbind();

	if(profiling_enabled_)
	{
		auto render_duration = query_timer_->stop();
		stats_.render_time = std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count();
	}

	// Update unused batches' time to live, remove dead batches
	int ii=0;
	for(; ii<s_max_batches; ++ii)
	{
		if(ii<=current_batch_)
			batch_ttl_[ii] = 0;
		else
			if(++batch_ttl_[ii]>s_max_batch_ttl)
				break;
	}
	remove_unused_batches(ii);
}

void Renderer2D::submit(const RenderState& state)
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
void Renderer2D::submit(WRef<VertexArray> va, hash_t shader_name, const ShaderParameters& params)
{
	const Shader& shader = shader_bank.get(shader_name);
	shader.bind();
	static_cast<const OGLShader&>(shader).send_uniform("u_view_projection"_h, scene_data_.view_projection_matrix);

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

void Renderer2D::draw_quad(const glm::vec2& position, 
				   		   const glm::vec2& scale,
				   		   const glm::vec4& uvs)
{
	// * Frustum culling
	if(frustum_cull(position, scale, scene_data_.frustum_sides)) return;

	// * Select a batch vertex array
	// Check that current batch has enough space, if not, upload batch and start to fill next batch
	if(current_batch_count_ == max_batch_count_)
	{
		upload_batch();

		// If current batch is the last one in list, add new batch
		if(current_batch_ >= get_num_batches()-1)
		{
			if(current_batch_ < s_max_batches-1)
				create_batch();
			else
			{
				DLOGW("render") << "[Renderer2D] Hitting max batch number." << std::endl;
				return;
			}
		}

		++current_batch_;
		current_batch_count_ = 0;
	}

	push_quad(position, scale, uvs);

	++current_batch_count_;

	W_ASSERT(Gfx::device->get_error()==0, "Driver error!");
}

// ----------------------------------------------------------------------------------------

BatchRenderer2D::BatchRenderer2D(uint32_t max_batch_count):
Renderer2D(max_batch_count)
{
	vertex_list_.reserve(max_batch_count * VERT_FLOAT_COUNT);

	for(int ii=0; ii<s_num_batches_init; ++ii)
		create_batch();

	// Load shader
	Renderer2D::shader_bank.load("shaders/color_dup_shader.glsl");
}

void BatchRenderer2D::create_batch()
{
	DLOGN("render") << "[BatchRenderer2D] Generating new batch." << std::endl;
	
	uint32_t num_vertices = max_batch_count_ * 3; // 3 vertices per triangle

	auto vb = VertexBuffer::create(nullptr, num_vertices, s_vertex_color_layout, DrawMode::Stream);
	auto va = VertexArray::create();
	va->set_vertex_buffer(vb);
	batches_.push_back(va);

	DLOG("render",1) << "New batch size is: " << batches_.size() << std::endl;
}

void BatchRenderer2D::upload_batch()
{
	// Map vertex list to GPU buffer
	batches_[current_batch_]->bind();
	auto& vb = batches_[current_batch_]->get_vertex_buffer();
	vb.map(vertex_list_.data(), vertex_list_.size());

	// Clear lists for next batches
	vertex_list_.clear();
}

void BatchRenderer2D::flush()
{
	const Shader& shader = Renderer2D::shader_bank.get("color_dup_shader"_h);
	shader.bind();

	static_cast<const OGLShader&>(shader).send_uniform("u_view_projection"_h, scene_data_.view_projection_matrix);

	uint32_t slot = shader.get_texture_slot("us_atlas"_h);
	scene_data_.texture->bind(slot);
	static_cast<const OGLShader&>(shader).send_uniform<int>("us_atlas"_h, slot);

	// Draw all full batches plus the last one if not empty
	for(int ii=0; ii<current_batch_; ++ii)
	{
    	Gfx::device->draw_array(batches_[ii], DrawPrimitive::Triangles, max_batch_count_*3);
		++stats_.batches;
	}
    if(current_batch_count_)
    {
    	Gfx::device->draw_array(batches_[current_batch_], DrawPrimitive::Triangles, current_batch_count_*3);
		++stats_.batches;
    }
	
	shader.unbind();
}

void BatchRenderer2D::remove_unused_batches(uint32_t index)
{
	if(index<s_max_batches)
		batches_.erase(batches_.begin()+index,batches_.end());
}

uint32_t BatchRenderer2D::get_num_batches()
{
	return batches_.size();
}

void BatchRenderer2D::push_quad(const glm::vec2& position, 
				   			    const glm::vec2& scale,
				   			    const glm::vec4& uvs)
{
	// * Push vertices and indices to current batch
	// We only need to store the lower right triangle as we can regenerate the other one in the geometry shader
	float vdata[VERT_FLOAT_COUNT] = 
	{
		position.x-0.5f*scale.x, position.y-0.5f*scale.y, 0.0f,   uvs.x, uvs.y,
		position.x+0.5f*scale.x, position.y-0.5f*scale.y, 0.0f,   uvs.z, uvs.y,
		position.x+0.5f*scale.x, position.y+0.5f*scale.y, 0.0f,   uvs.z, uvs.w,
	};

	vertex_list_.insert(vertex_list_.end(), vdata, vdata + VERT_FLOAT_COUNT);
}

void BatchRenderer2D::set_batch_size(uint32_t value)
{
	if(value<200)
		return;

	// Avoid lowering the batch size when the number of batches is already maximal
	uint32_t num_batches = batches_.size();
	if(num_batches>=s_max_batches && value<max_batch_count_)
		return;

	max_batch_count_ = value;
	batches_.clear();
	for(int ii=0; ii<num_batches; ++ii)
		create_batch();

	vertex_list_.clear();
	vertex_list_.reserve(max_batch_count_);

	current_batch_ = 0;
}

// ----------------------------------------------------------------------------------------

InstanceRenderer2D::InstanceRenderer2D(uint32_t max_batch_count):
Renderer2D(max_batch_count)
{
	instance_data_.reserve(max_batch_count_);

	// Load shader
	Renderer2D::shader_bank.load("shaders/color_inst_shader.glsl");

	// Create vertex array with a quad
	BufferLayout vertex_tex_layout =
	{
	    {"a_position"_h, ShaderDataType::Vec3},
	    {"a_uv"_h,       ShaderDataType::Vec2},
	};
	float sq_vdata[20] = 
	{
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f
	};
	uint32_t sq_idata[6] =
	{
		0, 1, 2,   2, 3, 0
	};
	auto quad_vb = VertexBuffer::create(sq_vdata, 20, vertex_tex_layout);
	auto quad_ib = IndexBuffer::create(sq_idata, 6, DrawPrimitive::Triangles);
	quad_va_ = VertexArray::create();
	quad_va_->set_index_buffer(quad_ib);
	quad_va_->set_vertex_buffer(quad_vb);

	for(int ii=0; ii<s_num_batches_init; ++ii)
		create_batch();
}

void InstanceRenderer2D::create_batch()
{
	DLOGN("render") << "[InstanceRenderer2D] Generating new batch." << std::endl;
	
	auto ssbo = ShaderStorageBuffer::create("instance_data", nullptr, max_batch_count_, sizeof(InstanceData), DrawMode::Dynamic);
	batches_.push_back(ssbo);

	DLOG("render",1) << "New batch size is: " << batches_.size() << std::endl;
}

void InstanceRenderer2D::upload_batch()
{
	// Map instance data to SSBO
	batches_[current_batch_]->map(instance_data_.data(), instance_data_.size());
	instance_data_.clear();
}

void InstanceRenderer2D::flush()
{
	const Shader& shader = Renderer2D::shader_bank.get("color_inst_shader"_h);
	shader.bind();
	static_cast<const OGLShader&>(shader).send_uniform("u_view_projection"_h, scene_data_.view_projection_matrix);

	uint32_t slot = shader.get_texture_slot("us_atlas"_h);
	scene_data_.texture->bind(slot);
	static_cast<const OGLShader&>(shader).send_uniform<int>("us_atlas"_h, slot);

	// Draw all full batches plus the last one if not empty
	for(int ii=0; ii<current_batch_; ++ii)
	{
		shader.attach_shader_storage(*batches_[ii], ii);
    	Gfx::device->draw_indexed_instanced(quad_va_, max_batch_count_);
		++stats_.batches;
	}
    if(current_batch_count_)
    {
		shader.attach_shader_storage(*batches_[current_batch_], current_batch_);
    	Gfx::device->draw_indexed_instanced(quad_va_, current_batch_count_);
		++stats_.batches;
    }
	
	shader.unbind();
}

void InstanceRenderer2D::remove_unused_batches(uint32_t index)
{
	if(index<s_max_batches)
		batches_.erase(batches_.begin()+index,batches_.end());
}

uint32_t InstanceRenderer2D::get_num_batches()
{
	return batches_.size();
}

void InstanceRenderer2D::push_quad(const glm::vec2& position, 
				   			       const glm::vec2& scale,
				   			       const glm::vec4& uvs)
{
	// Push instance data
	instance_data_.push_back({position, scale, uvs});
}

void InstanceRenderer2D::set_batch_size(uint32_t value)
{
	if(value<200)
		return;

	// Avoid lowering the batch size when the number of batches is already maximal
	uint32_t num_batches = batches_.size();
	if(num_batches>=s_max_batches && value<max_batch_count_)
		return;

	max_batch_count_ = value;

	batches_.clear();
	for(int ii=0; ii<num_batches; ++ii)
		create_batch();

	instance_data_.clear();
	instance_data_.reserve(max_batch_count_);

	current_batch_ = 0;
}

} // namespace erwin