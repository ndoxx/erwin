#include <map>

#include "render/renderer_2d.h"
#include "core/config.h"
#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{
struct InstanceData // Need correct alignment for SSBO data
{
	glm::vec4 uvs;
	glm::vec4 tint;
	glm::vec4 offset;
	glm::vec2 scale;
	glm::vec2 padding;
};

struct Batch2D
{
	TextureHandle texture;
	uint32_t count;
	float max_depth;
	// std::vector<InstanceData> instance_data;
	InstanceData* instance_data;
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
	TextureHandle white_texture;
	uint32_t white_texture_data;

	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
	FrustumSides frustum_sides;
	uint64_t state_flags;

	uint32_t num_draw_calls; // stats
	uint32_t max_batch_count;
	uint8_t layer_id;
	std::map<hash_t, Batch2D> batches;
};
static Renderer2DStorage storage;

static void create_batch(hash_t atlas_name, TextureHandle texture)
{
	storage.batches.insert(std::make_pair(atlas_name, Batch2D()));
	auto& batch = storage.batches[atlas_name];
	// batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, storage.max_batch_count, MainRenderer::get_arena());
	batch.count = 0;
	batch.max_depth = -1.f;
	batch.texture = texture;
}

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

void Renderer2D::init()
{
	storage.num_draw_calls = 0;
	storage.max_batch_count = cfg::get<uint32_t>("erwin.renderer.max_2d_batch_count"_h, 8192);

	float sq_vdata[20] = 
	{
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	uint32_t index_data[6] = { 0, 1, 2, 2, 3, 0 };

	storage.shader_handle = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/instance_shader.glsl", "instance_shader");
	// storage.shader_handle = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/instance_shader.spv", "instance_shader");
	storage.ibo_handle = MainRenderer::create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
	storage.vbl_handle = MainRenderer::create_vertex_buffer_layout({
			    				 			    	{"a_position"_h, ShaderDataType::Vec3},
								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
								 			    });
	storage.vbo_handle = MainRenderer::create_vertex_buffer(storage.vbl_handle, sq_vdata, 20, DrawMode::Static);
	storage.va_handle = MainRenderer::create_vertex_array(storage.vbo_handle, storage.ibo_handle);
	storage.ubo_handle = MainRenderer::create_uniform_buffer("matrices", nullptr, sizeof(glm::mat4), DrawMode::Dynamic);
	storage.ssbo_handle = MainRenderer::create_shader_storage_buffer("instance_data", nullptr, storage.max_batch_count*sizeof(InstanceData), DrawMode::Dynamic);
	
	storage.white_texture_data = 0xffffffff;
	storage.white_texture = MainRenderer::create_texture_2D(Texture2DDescriptor{1,1,
								  					 				   			&storage.white_texture_data,
								  					 				   			ImageFormat::RGBA8,
								  					 				   			MAG_NEAREST | MIN_NEAREST});
	create_batch(0, storage.white_texture);

	MainRenderer::shader_attach_uniform_buffer(storage.shader_handle, storage.ubo_handle);
	MainRenderer::shader_attach_storage_buffer(storage.shader_handle, storage.ssbo_handle);
}

void Renderer2D::shutdown()
{
	MainRenderer::destroy_texture_2D(storage.white_texture);
	MainRenderer::destroy_shader_storage_buffer(storage.ssbo_handle);
	MainRenderer::destroy_uniform_buffer(storage.ubo_handle);
	MainRenderer::destroy_vertex_array(storage.va_handle);
	MainRenderer::destroy_vertex_buffer(storage.vbo_handle);
	MainRenderer::destroy_vertex_buffer_layout(storage.vbl_handle);
	MainRenderer::destroy_index_buffer(storage.ibo_handle);
	MainRenderer::destroy_shader(storage.shader_handle);
}

void Renderer2D::register_atlas(hash_t name, TextureAtlas& atlas)
{
	ImageFormat format;
	switch(atlas.descriptor.texture_compression)
	{
		case TextureCompression::None: format = ImageFormat::SRGB_ALPHA; break;
		case TextureCompression::DXT1: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1; break;
		case TextureCompression::DXT5: format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5; break;
	}
	uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
	// uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

	atlas.handle = MainRenderer::create_texture_2D(Texture2DDescriptor{atlas.descriptor.texture_width,
								  					 				   atlas.descriptor.texture_height,
								  					 				   atlas.descriptor.texture_blob,
								  					 				   format,
								  					 				   filter});

	create_batch(name, atlas.handle);
}

void Renderer2D::begin_pass(FramebufferHandle render_target, const PassState& state, const OrthographicCamera2D& camera, uint8_t layer_id)
{
	// Reset stats
	storage.num_draw_calls = 0;

	storage.state_flags = state.encode();
	auto& rq = MainRenderer::get_queue(0);
	rq.set_render_target(render_target);

	storage.layer_id = layer_id;

	// Set scene data
	storage.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_sides = camera.get_frustum_sides();

	// Reset batch instance data pointers
	for(auto&& [key, batch]: storage.batches)
		batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, storage.max_batch_count, MainRenderer::get_arena());
}

void Renderer2D::end_pass()
{
	Renderer2D::flush();
}

static void flush_batch(Batch2D& batch)
{
	if(batch.count)
	{
		auto& rq = MainRenderer::get_queue(0);

		DrawCall dc(rq, DrawCall::IndexedInstanced, storage.shader_handle, storage.va_handle);
		dc.set_state(storage.state_flags);
		dc.set_per_instance_UBO(storage.ubo_handle, &storage.view_projection_matrix, sizeof(glm::mat4));
		dc.set_instance_data_SSBO(storage.ssbo_handle, batch.instance_data, batch.count * sizeof(InstanceData), batch.count);
		dc.set_texture("us_atlas"_h, batch.texture);
		dc.set_key_depth(batch.max_depth, storage.layer_id);
		dc.submit();

		++storage.num_draw_calls;
		batch.count = 0;
		batch.max_depth = -1.f;
		batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, storage.max_batch_count, MainRenderer::get_arena());
	}
}

void Renderer2D::draw_quad(const glm::vec4& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas, const glm::vec4& tint)
{
	// * Frustum culling
	if(frustum_cull(glm::vec2(position), scale, storage.frustum_sides)) return;

	// Get appropriate batch
	auto& batch = storage.batches[atlas];

	// Check that current batch has enough space, if not, upload batch and start to fill next batch
	if(batch.count == storage.max_batch_count)
		flush_batch(batch);

	// Set batch depth as the maximal algebraic quad depth (camera looking along negative z axis)
	if(position.z > batch.max_depth)
		batch.max_depth = position.z; // TMP: this must be in view space

	batch.instance_data[batch.count] = {uvs, tint, position, scale};
	++batch.count;
}

void Renderer2D::flush()
{
	for(auto&& [key, batch]: storage.batches)
		flush_batch(batch);
}

uint32_t Renderer2D::get_draw_call_count()
{
	return storage.num_draw_calls;
}

} // namespace erwin