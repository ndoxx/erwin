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
	glm::vec4 eye_position;
	glm::vec4 camera_params;
	glm::vec4 framebuffer_size; // x,y: framebuffer dimensions in pixels, z: aspect ratio, w: padding
	glm::vec4 light_position;
	glm::vec4 light_color;
	glm::vec4 light_ambient_color;
	float light_ambient_strength;
};

struct InstanceData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

static struct ForwardRenderer3DStorage
{
	// Resources
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;

	// Data
	PassUBOData pass_ubo_data;
	FrustumPlanes frustum_planes;

	// State
	uint64_t pass_state;
	uint8_t layer_id;
	bool draw_far;

	// Statistics
	uint32_t num_draw_calls;
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
	if(material_ubo.index != k_invalid_handle)
		MainRenderer::shader_attach_uniform_buffer(shader, material_ubo);
}

void ForwardRenderer::begin_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light, PassOptions options)
{
    W_PROFILE_FUNCTION()

	// Pass state
	PassState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_forward"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = options.get_transparency() ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;
	state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

	s_storage.pass_state = state.encode();
	s_storage.layer_id = options.get_layer_id();
	s_storage.draw_far = (options.get_depth_control() == PassOptions::DEPTH_CONTROL_FAR);

	// Reset stats
	s_storage.num_draw_calls = 0;

	// TMP
	MainRenderer::get_queue("ForwardOpaque"_h).set_clear_color(state.rasterizer_state.clear_color);

	// Set scene data
	glm::vec2 fb_size = FramebufferPool::get_size("fb_forward"_h);
	float near = camera.get_frustum().near;
	float far  = camera.get_frustum().far;

	s_storage.pass_ubo_data.view_matrix = camera.get_view_matrix();
	s_storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	s_storage.pass_ubo_data.eye_position = glm::vec4(camera.get_position(), 1.f);
	s_storage.pass_ubo_data.camera_params = glm::vec4(near,far,0.f,0.f);
	s_storage.pass_ubo_data.framebuffer_size = glm::vec4(fb_size, fb_size.x/fb_size.y, 0.f);
	s_storage.pass_ubo_data.light_position = glm::vec4(dir_light.position, 0.f);
	s_storage.pass_ubo_data.light_color = glm::vec4(dir_light.color, 1.f) * dir_light.brightness;
	s_storage.pass_ubo_data.light_ambient_color = glm::vec4(dir_light.ambient_color, 1.f);
	s_storage.pass_ubo_data.light_ambient_strength = dir_light.ambient_strength;
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
	if(material.ubo.index != k_invalid_handle && material.data)
	{
		dc.set_UBO(material.ubo, material.data, material.data_size, DrawCall::CopyData, 1);
	}
	if(material.texture_group.index != k_invalid_handle)
	{
		const TextureGroup& tg = AssetManager::get(material.texture_group);
		for(uint32_t ii=0; ii<tg.texture_count; ++ii)
			dc.set_texture(tg.textures[ii], ii);
	}

	dc.set_key_depth(depth, s_storage.layer_id);
	// MainRenderer::submit(PassState::is_transparent(s_storage.pass_state) ? "ForwardTransparent"_h : "ForwardOpaque"_h, dc);
	MainRenderer::submit("ForwardOpaque"_h, dc);

	++s_storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return s_storage.num_draw_calls;
}


} // namespace erwin