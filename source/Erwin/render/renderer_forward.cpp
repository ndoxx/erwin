#include "render/renderer_forward.h"
#include "render/common_geometry.h"
#include "render/main_renderer.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace erwin
{

struct PassUBOData
{
	glm::mat4 view_projection_matrix;
	glm::mat4 view_matrix;
};

struct InstanceData
{
	glm::mat4 mvp;
	glm::mat4 mv;
	glm::mat4 m;
};

struct ForwardRenderer3DStorage
{
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;

	PassUBOData pass_ubo_data;

	glm::mat4 view_matrix;
	FrustumPlanes frustum_planes;
	uint64_t pass_state;

	uint32_t num_draw_calls; // stats
	uint8_t layer_id;
};
static ForwardRenderer3DStorage storage;

void ForwardRenderer::init()
{
    W_PROFILE_FUNCTION()

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_forward"_h, make_scope<FbRatioConstraint>(), layout, true);

	storage.num_draw_calls = 0;

	storage.instance_ubo = MainRenderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	storage.pass_ubo     = MainRenderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
}

void ForwardRenderer::shutdown()
{
	MainRenderer::destroy(storage.instance_ubo);
	MainRenderer::destroy(storage.pass_ubo);
}

void ForwardRenderer::register_shader(ShaderHandle shader, UniformBufferHandle material_ubo)
{
	MainRenderer::shader_attach_uniform_buffer(shader, storage.pass_ubo);
	MainRenderer::shader_attach_uniform_buffer(shader, storage.instance_ubo);
	MainRenderer::shader_attach_uniform_buffer(shader, material_ubo);
}

void ForwardRenderer::begin_pass(const PerspectiveCamera3D& camera, bool transparent, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()

	PassState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_forward"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = transparent ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;
	state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

	// Reset stats
	storage.num_draw_calls = 0;

	// Pass state
	storage.pass_state = state.encode();
	storage.layer_id = layer_id;
	MainRenderer::get_queue("ForwardOpaque"_h).set_clear_color(state.rasterizer_state.clear_color); // TMP

	// Set scene data
	storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	storage.pass_ubo_data.view_matrix = camera.get_view_matrix();

	storage.view_matrix = camera.get_view_matrix();
	storage.frustum_planes = camera.get_frustum_planes();

	MainRenderer::update_uniform_buffer(storage.pass_ubo, &storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material)
{
	W_ASSERT_FMT(VAO.is_valid(), "Invalid VertexArrayHandle of index %hu.", VAO.index);

	glm::mat4 model_matrix = transform.get_model_matrix();
	InstanceData instance_data;
	// TODO: tint should be a material property
	instance_data.mvp = storage.pass_ubo_data.view_projection_matrix 
				      * model_matrix;
	instance_data.mv  = storage.pass_ubo_data.view_matrix 
				      * model_matrix;
	instance_data.m   = model_matrix;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	
	static DrawCall dc(DrawCall::Indexed, material.shader, VAO);
	dc.set_state(storage.pass_state);
	dc.set_UBO(storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData), DrawCall::CopyData, 0);
	dc.set_UBO(material.ubo, material.data, material.data_size, DrawCall::CopyData, 1);
	dc.set_key_depth(depth, storage.layer_id);
	const TextureGroup& tg = AssetManager::get(material.texture_group);
	for(uint32_t ii=0; ii<tg.texture_count; ++ii)
		dc.set_texture(tg.textures[ii], ii);
	MainRenderer::submit("ForwardOpaque"_h, dc); // TODO: handle transparency

	++storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return storage.num_draw_calls;
}


} // namespace erwin