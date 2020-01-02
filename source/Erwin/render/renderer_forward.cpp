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
	glm::mat4 view_matrix;
	glm::mat4 view_projection_matrix;
};

struct InstanceData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

static struct ForwardRenderer3DStorage
{
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;

	PassUBOData pass_ubo_data;

	FrustumPlanes frustum_planes;
	uint64_t pass_state;

	uint32_t num_draw_calls; // stats
	uint8_t layer_id;
} s_storage;

void ForwardRenderer::init()
{
    W_PROFILE_FUNCTION()

    FramebufferLayout layout =
    {
        {"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
    };
    FramebufferPool::create_framebuffer("fb_forward"_h, make_scope<FbRatioConstraint>(), layout, true);

	s_storage.num_draw_calls = 0;

	s_storage.instance_ubo = MainRenderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	s_storage.pass_ubo     = MainRenderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
}

void ForwardRenderer::shutdown()
{
	MainRenderer::destroy(s_storage.instance_ubo);
	MainRenderer::destroy(s_storage.pass_ubo);
}

void ForwardRenderer::register_shader(ShaderHandle shader, UniformBufferHandle material_ubo)
{
	MainRenderer::shader_attach_uniform_buffer(shader, s_storage.pass_ubo);
	MainRenderer::shader_attach_uniform_buffer(shader, s_storage.instance_ubo);
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
	s_storage.num_draw_calls = 0;

	// Pass state
	s_storage.pass_state = state.encode();
	s_storage.layer_id = layer_id;

	// TMP
	// if(transparent)
		// MainRenderer::get_queue("ForwardTransparent"_h).set_clear_color(state.rasterizer_state.clear_color);
	// else
		MainRenderer::get_queue("ForwardOpaque"_h).set_clear_color(state.rasterizer_state.clear_color);

	// Set scene data
	s_storage.pass_ubo_data.view_matrix = camera.get_view_matrix();
	s_storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	s_storage.frustum_planes = camera.get_frustum_planes();

	MainRenderer::update_uniform_buffer(s_storage.pass_ubo, &s_storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material)
{
	W_ASSERT_FMT(VAO.is_valid(), "Invalid VertexArrayHandle of index %hu.", VAO.index);

	// Compute matrices
	InstanceData instance_data;
	instance_data.m   = transform.get_model_matrix();
	instance_data.mv  = s_storage.pass_ubo_data.view_matrix * instance_data.m;
	instance_data.mvp = s_storage.pass_ubo_data.view_projection_matrix * instance_data.m;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	
	DrawCall dc(DrawCall::Indexed, material.shader, VAO);
	dc.set_state(s_storage.pass_state);
	dc.set_UBO(s_storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData), DrawCall::CopyData, 0);
	dc.set_UBO(material.ubo, material.data, material.data_size, DrawCall::CopyData, 1);
	dc.set_key_depth(depth, s_storage.layer_id);
	const TextureGroup& tg = AssetManager::get(material.texture_group);
	for(uint32_t ii=0; ii<tg.texture_count; ++ii)
		dc.set_texture(tg.textures[ii], ii);
	// MainRenderer::submit(PassState::is_transparent(s_storage.pass_state) ? "ForwardTransparent"_h : "ForwardOpaque"_h, dc);
	MainRenderer::submit("ForwardOpaque"_h, dc);

	++s_storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return s_storage.num_draw_calls;
}


} // namespace erwin