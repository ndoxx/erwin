#include <map>

#include "render/renderer_2d.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "asset/asset_manager.h"
#include "asset/texture_atlas.h"
#include "core/config.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/vec_swizzle.hpp"

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
	InstanceData* instance_data;
};

struct Renderer2DStorage
{
	ShaderHandle batch_2d_shader;
	UniformBufferHandle pass_ubo;
	ShaderStorageBufferHandle instance_ssbo;
	TextureHandle white_texture;
	uint32_t white_texture_data;

	glm::mat4 view_projection_matrix;
	glm::mat4 projection_matrix;
	glm::mat4 view_matrix;
	glm::vec2 fb_size;
	FrustumSides frustum_sides;
	uint64_t pass_state;

	uint32_t num_draw_calls; // stats
	uint32_t max_batch_count;
	uint8_t layer_id;
	std::map<uint16_t, Batch2D> batches;
};
static Renderer2DStorage storage;

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

static void create_batch(uint16_t index, TextureHandle handle)
{
	storage.batches.insert(std::make_pair(index, Batch2D()));

	auto& batch = storage.batches[index];
	batch.count = 0;
	batch.max_depth = -1.f;
	batch.texture = handle;
}

void Renderer2D::init()
{
    W_PROFILE_FUNCTION()

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_2d_raw"_h, make_scope<FbRatioConstraint>(), layout, true);

	storage.num_draw_calls = 0;
	storage.max_batch_count = cfg::get<uint32_t>("erwin.renderer.max_2d_batch_count"_h, 8192);

	// storage.batch_2d_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/instance_shader.glsl", "instance_shader");
	storage.batch_2d_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/instance_shader.spv", "instance_shader");
	storage.pass_ubo = Renderer::create_uniform_buffer("matrices", nullptr, sizeof(glm::mat4), DrawMode::Dynamic);
	storage.instance_ssbo = Renderer::create_shader_storage_buffer("instance_data", nullptr, storage.max_batch_count*sizeof(InstanceData), DrawMode::Dynamic);
	
	Renderer::shader_attach_uniform_buffer(storage.batch_2d_shader, storage.pass_ubo);
	Renderer::shader_attach_storage_buffer(storage.batch_2d_shader, storage.instance_ssbo);

	storage.white_texture_data = 0xffffffff;
	storage.white_texture = Renderer::create_texture_2D(Texture2DDescriptor{1,1,
								  					 				   			&storage.white_texture_data,
								  					 				   			ImageFormat::RGBA8,
								  					 				   			MAG_NEAREST | MIN_NEAREST});
	::erwin::create_batch(0, storage.white_texture);
}

void Renderer2D::shutdown()
{
    W_PROFILE_FUNCTION()

	Renderer::destroy(storage.white_texture);
	Renderer::destroy(storage.instance_ssbo);
	Renderer::destroy(storage.pass_ubo);
	Renderer::destroy(storage.batch_2d_shader);
}

void Renderer2D::create_batch(TextureHandle handle)
{
	::erwin::create_batch(handle.index, handle);
}

void Renderer2D::begin_pass(const OrthographicCamera2D& camera, bool transparent, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()

	PassState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_2d_raw"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = transparent ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;
	state.rasterizer_state.clear_color = glm::vec4(0.0f,0.0f,0.0f,0.f);

	// Reset stats
	storage.num_draw_calls = 0;

	storage.pass_state = state.encode();
	storage.layer_id = layer_id;

	// Set scene data
	storage.view_projection_matrix = camera.get_view_projection_matrix();
	storage.view_matrix = camera.get_view_matrix();
	storage.projection_matrix = camera.get_projection_matrix();
	storage.frustum_sides = camera.get_frustum_sides();
	storage.fb_size = FramebufferPool::get_screen_size();

	// Reset batch instance data pointers
	for(auto&& [key, batch]: storage.batches)
		batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, storage.max_batch_count, Renderer::get_arena());
}

void Renderer2D::end_pass()
{
    W_PROFILE_FUNCTION()
    
	Renderer2D::flush();
}

static void flush_batch(Batch2D& batch)
{
	if(batch.count)
	{
		static DrawCall dc(DrawCall::IndexedInstanced, storage.pass_state, storage.batch_2d_shader, CommonGeometry::get_vertex_array("quad"_h));
		dc.set_UBO(storage.pass_ubo, &storage.view_projection_matrix, sizeof(glm::mat4), DrawCall::CopyData);
		dc.set_SSBO(storage.instance_ssbo, batch.instance_data, batch.count * sizeof(InstanceData), batch.count, DrawCall::ForwardData);
		dc.set_texture(batch.texture);
		dc.set_key_depth(batch.max_depth, storage.layer_id);
		Renderer::submit(dc);

		++storage.num_draw_calls;
		batch.count = 0;
		batch.max_depth = -1.f;
		batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, storage.max_batch_count, Renderer::get_arena());
	}
}

void Renderer2D::draw_quad(const ComponentTransform2D& transform, TextureAtlasHandle atlas_handle, hash_t tile, const glm::vec4& tint)
{
	// * Frustum culling
	if(frustum_cull(glm::xy(transform.position), glm::vec2(transform.uniform_scale), storage.frustum_sides)) return;

	// Get appropriate batch
	const TextureAtlas& atlas = AssetManager::get(atlas_handle);
	auto& batch = storage.batches[atlas.texture.index];

	// Check that current batch has enough space, if not, upload batch and start to fill next batch
	if(batch.count == storage.max_batch_count)
		flush_batch(batch);

	// Set batch depth as the maximal algebraic quad depth (camera looking along negative z axis)
	if(transform.position.z > batch.max_depth)
		batch.max_depth = transform.position.z; // TMP: this must be in view space

	glm::vec4 uvs = atlas.get_uv(tile);
	batch.instance_data[batch.count] = {uvs, tint, glm::vec4(transform.position, 1.f), glm::vec2(transform.uniform_scale)};
	++batch.count;
}

void Renderer2D::draw_colored_quad(const ComponentTransform2D& transform, const glm::vec4& tint)
{
	// * Frustum culling
	if(frustum_cull(glm::xy(transform.position), glm::vec2(transform.uniform_scale), storage.frustum_sides)) return;

	// Get appropriate batch
	auto& batch = storage.batches[0];

	// Check that current batch has enough space, if not, upload batch and start to fill next batch
	if(batch.count == storage.max_batch_count)
		flush_batch(batch);

	// Set batch depth as the maximal algebraic quad depth (camera looking along negative z axis)
	if(transform.position.z > batch.max_depth)
		batch.max_depth = transform.position.z; // TMP: this must be in view space

	batch.instance_data[batch.count] = {{0.f,0.f,1.f,1.f}, tint, glm::vec4(transform.position, 1.f), glm::vec2(transform.uniform_scale)};
	++batch.count;
}

void Renderer2D::draw_text(const std::string& text, FontAtlasHandle font_handle, float x, float y, float scale, const glm::vec4& tint)
{
	// Ad hoc rescaling of character advance parameter
	constexpr float k_adv_factor = 1.05f;

	// Get font atlas
	const FontAtlas& font = AssetManager::get(font_handle);

	Batch2D batch;
	batch.instance_data = W_NEW_ARRAY_DYNAMIC(InstanceData, text.size(), Renderer::get_arena());
	batch.max_depth = 0.f;
	batch.texture = font.texture;
	batch.count = 0;

    float ar = storage.fb_size.x / storage.fb_size.y;

	std::string::const_iterator itc;
    for(itc = text.begin(); itc != text.end(); ++itc)
    {
    	const FontAtlas::RemappingElement& remap = font.get_remapping(*itc);
    	// Handle null size characters
    	if(remap.w == 0)
    	{
    		x += k_adv_factor*scale*remap.advance / storage.fb_size.y;
    		continue;
    	}

    	// NOTE: bearing_y is modified in FontAtlas to properly offset lower than baseline characters
    	float xpos = x + scale*(remap.bearing_x+0.5f*remap.w)/storage.fb_size.y;
    	float ypos = y + scale*(remap.bearing_y)/storage.fb_size.y;

    	glm::vec2 vscale = {scale*remap.w/storage.fb_size.x, scale*remap.h/storage.fb_size.y};

    	batch.instance_data[batch.count++] = {remap.uvs, tint, glm::vec4(xpos, ypos, 0.f, 1.f), vscale};

    	x += k_adv_factor*scale*remap.advance / storage.fb_size.y;
    }

	if(batch.count)
	{
		glm::mat4 id(1.f);

		static DrawCall dc(DrawCall::IndexedInstanced, storage.pass_state, storage.batch_2d_shader, CommonGeometry::get_vertex_array("quad"_h));
		dc.set_UBO(storage.pass_ubo, &id, sizeof(glm::mat4), DrawCall::CopyData);
		dc.set_SSBO(storage.instance_ssbo, batch.instance_data, batch.count * sizeof(InstanceData), batch.count, DrawCall::ForwardData);
		dc.set_texture(batch.texture);
		dc.set_key_depth(batch.max_depth, storage.layer_id);
		Renderer::submit(dc);

		++storage.num_draw_calls;
	}
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