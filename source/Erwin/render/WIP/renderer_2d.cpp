#include "render/WIP/renderer_2d.h"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{
namespace WIP
{

struct InstanceData // Need correct alignment for SSBO data
{
	glm::vec2 offset;
	glm::vec2 scale;
	glm::vec4 uvs;
};

struct Renderer2DStorage
{
	IndexBufferHandle ibo_handle;
	VertexBufferLayoutHandle vbl_handle;
	VertexBufferHandle vbo_handle;
	VertexArrayHandle va_handle;
	ShaderHandle shader_handle;
	UniformBufferHandle ubo_handle;
	ShaderStorageBufferHandle ssbo_handle;

	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
	FrustumSides frustum_sides;

	uint32_t num_batches; // stats
	uint32_t max_batch_count;
	uint32_t current_batch_count;
	std::vector<InstanceData> instance_data;
};

// TMP: MOVE this to proper collision trait class?
static bool frustum_cull(const glm::vec2& position, const glm::vec2& scale, const FrustumSides& fs)
{
	// Compute each point in world space
	glm::vec3 points[4] =
	{
		// WTF: Scale terms should be multiplied by 0.5f to make sense,
		// but it produces false positives at the edges.
		glm::vec3(position.x-scale.x, position.y-scale.y, 1.0f),
		glm::vec3(position.x+scale.x, position.y-scale.y, 1.0f),
		glm::vec3(position.x+scale.x, position.y+scale.y, 1.0f),
		glm::vec3(position.x-scale.x, position.y+scale.y, 1.0f)
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

static Renderer2DStorage storage;

void Renderer2D::init(uint32_t max_batch_count)
{
	storage.num_batches = 1;
	storage.current_batch_count = 0;
	storage.max_batch_count = max_batch_count;
	storage.instance_data.reserve(max_batch_count);

	float sq_vdata[20] = 
	{
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	uint32_t index_data[6] = { 0, 1, 2, 2, 3, 0 };

	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	storage.shader_handle = rq.create_shader(filesystem::get_asset_dir() / "shaders/instance_shader.spv", "instance_shader");
	storage.ibo_handle = rq.create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
	storage.vbl_handle = rq.create_vertex_buffer_layout({
			    				 			    	{"a_position"_h, ShaderDataType::Vec3},
								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
								 			    });
	storage.vbo_handle = rq.create_vertex_buffer(storage.vbl_handle, sq_vdata, 20, DrawMode::Static);
	storage.va_handle = rq.create_vertex_array(storage.vbo_handle, storage.ibo_handle);
	storage.ubo_handle = rq.create_uniform_buffer("matrices", nullptr, sizeof(glm::mat4), DrawMode::Dynamic);
	storage.ssbo_handle = rq.create_shader_storage_buffer("instance_data", nullptr, max_batch_count*sizeof(InstanceData), DrawMode::Dynamic);


	rq.shader_attach_uniform_buffer(storage.shader_handle, storage.ubo_handle);
	rq.shader_attach_storage_buffer(storage.shader_handle, storage.ssbo_handle);
}

void Renderer2D::shutdown()
{
	auto& rq = MainRenderer::get_queue(MainRenderer::Resource);
	rq.destroy_shader_storage_buffer(storage.ssbo_handle);
	rq.destroy_uniform_buffer(storage.ubo_handle);
	rq.destroy_vertex_array(storage.va_handle);
	rq.destroy_vertex_buffer(storage.vbo_handle);
	rq.destroy_vertex_buffer_layout(storage.vbl_handle);
	rq.destroy_index_buffer(storage.ibo_handle);
	rq.destroy_shader(storage.shader_handle);
}

void Renderer2D::begin_pass(const PassState& state, const OrthographicCamera2D& camera)
{
	// Reset stats
	storage.num_batches = 1;

	auto& rq = MainRenderer::get_queue(MainRenderer::Opaque);
	rq.set_state(state);

	// Set scene data
	storage.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_sides = camera.get_frustum_sides();
}

void Renderer2D::end_pass()
{

}

void Renderer2D::draw_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas)
{
	// * Frustum culling
	if(frustum_cull(position, scale, storage.frustum_sides)) return;

	// Check that current batch has enough space, if not, upload batch and start to fill next batch
	if(storage.current_batch_count == storage.max_batch_count)
	{
		flush();
		++storage.num_batches;
		storage.current_batch_count = 0;
	}

	storage.instance_data[storage.current_batch_count] = {position, scale, uvs};
	++storage.current_batch_count;
}

void Renderer2D::flush()
{
	auto& rq = MainRenderer::get_queue(MainRenderer::Opaque);

	DrawCall dc(rq, DrawCall::IndexedInstanced, storage.shader_handle, storage.va_handle);
	dc.set_per_instance_UBO(storage.ubo_handle, &storage.view_projection_matrix, sizeof(glm::mat4));
	dc.set_instance_data_SSBO(storage.ssbo_handle, storage.instance_data.data(), storage.current_batch_count * sizeof(InstanceData), storage.current_batch_count);
	dc.submit();
}

} // namespace WIP
} // namespace erwin