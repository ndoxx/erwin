#include "render/WIP_renderer_2d.h"
#include "render/master_renderer.h"
#include "debug/logger.h"

namespace erwin
{
namespace WIP // Work in progress ;)
{

static const uint32_t s_num_batches_init = 1;
static const uint32_t s_max_batches = 32;
static const uint16_t s_max_batch_ttl = 60*2;

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
current_batch_count_(0)
{
	query_timer_ = QueryTimer::create();

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
	quad_va_ = VertexArray::create();
	quad_va_->set_index_buffer(quad_ib);
	quad_va_->set_vertex_buffer(quad_vb);
	mat_ubo_ = UniformBuffer::create("matrices_layout", nullptr, sizeof(glm::mat4), DrawMode::Dynamic);

	for(int ii=0; ii<s_num_batches_init; ++ii)
		create_batch();
}

Renderer2D::~Renderer2D()
{

}

void Renderer2D::begin_scene(const PassState& render_state, const OrthographicCamera2D& camera, WRef<Texture2D> texture)
{
	// Set render state
	render_state_ = render_state;

	// Set scene data
	scene_data_.view_projection_matrix = camera.get_view_projection_matrix();
	scene_data_.view_matrix = camera.get_view_matrix();
	scene_data_.frustum_sides = camera.get_frustum_sides();
	scene_data_.texture = texture;

	// Reset
	current_batch_count_ = 0;
	reset_stats();
}

void Renderer2D::end_scene()
{
	upload_batch();

	if(profiling_enabled_)
		query_timer_->start();

	// Flush
	mat_ubo_->map(&scene_data_.view_projection_matrix);
	auto& queue = MASTER_RENDERER->get_queue<InstancedSpriteQueueData>();
	auto* state = queue.pass_state_ptr();
	*state = render_state_;
	queue.begin_pass(state);

	for(int ii=0; ii<=current_batch_; ++ii)
	{
		auto* data = queue.data_ptr();
		data->instance_count = (ii==current_batch_) ? current_batch_count_ : max_batch_count_;
		data->texture = scene_data_.texture;
		data->VAO = quad_va_;
		data->UBO = mat_ubo_;
		data->SSBO = batches_[ii];
		queue.push(data);

		++stats_.batches;
	}

	// TMP
	MASTER_RENDERER->flush();

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
		if(current_batch_ >= batches_.size()-1)
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

	instance_data_.push_back({position, scale, uvs});

	++current_batch_count_;
}

void Renderer2D::set_batch_size(uint32_t value)
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

void Renderer2D::create_batch()
{
	DLOGN("render") << "[Renderer2D] Generating new batch." << std::endl;
	
	auto ssbo = ShaderStorageBuffer::create("instance_data", nullptr, max_batch_count_, sizeof(InstanceData), DrawMode::Dynamic);
	batches_.push_back(ssbo);

	DLOG("render",1) << "New batch size is: " << batches_.size() << std::endl;
}

void Renderer2D::upload_batch()
{
	// Map instance data to SSBO
	batches_[current_batch_]->map(instance_data_.data(), instance_data_.size());
	instance_data_.clear();
}

void Renderer2D::remove_unused_batches(uint32_t index)
{
	if(index<s_max_batches)
		batches_.erase(batches_.begin()+index,batches_.end());
}


} // namespace WIP
} // namespace erwin