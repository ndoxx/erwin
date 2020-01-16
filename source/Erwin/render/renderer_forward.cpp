#include "render/renderer_forward.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
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
	glm::vec4 proj_params;
	float light_ambient_strength;
};

struct InstanceData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

static struct
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

	// Setup UBOs and init storage
	s_storage.instance_ubo = Renderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	s_storage.pass_ubo     = Renderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
	s_storage.num_draw_calls = 0;
}

void ForwardRenderer::shutdown()
{
	Renderer::destroy(s_storage.pass_ubo);
	Renderer::destroy(s_storage.instance_ubo);
}

void ForwardRenderer::register_shader(ShaderHandle shader, UniformBufferHandle material_ubo)
{
	Renderer::shader_attach_uniform_buffer(shader, s_storage.pass_ubo);
	Renderer::shader_attach_uniform_buffer(shader, s_storage.instance_ubo);
	if(material_ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(shader, material_ubo);
}

void ForwardRenderer::begin_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light, PassOptions options)
{
    W_PROFILE_FUNCTION()

	// Pass state
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = options.get_transparency() ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	s_storage.pass_state = state.encode();
	s_storage.layer_id = Renderer::next_view_id();
	s_storage.draw_far = (options.get_depth_control() == PassOptions::DEPTH_CONTROL_FAR);

	// Reset stats
	s_storage.num_draw_calls = 0;

	// Set scene data
	glm::vec2 fb_size = FramebufferPool::get_screen_size();
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
	s_storage.pass_ubo_data.proj_params = camera.get_projection_parameters();
	s_storage.pass_ubo_data.light_ambient_strength = dir_light.ambient_strength;
	s_storage.frustum_planes = camera.get_frustum_planes();

	Renderer::update_uniform_buffer(s_storage.pass_ubo, &s_storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material)
{
	// Compute matrices
	InstanceData instance_data;
	instance_data.m   = transform.get_model_matrix();
	instance_data.mv  = s_storage.pass_ubo_data.view_matrix * instance_data.m;
	instance_data.mvp = s_storage.pass_ubo_data.view_projection_matrix * instance_data.m;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	
	DrawCall dc(DrawCall::Indexed, s_storage.layer_id, s_storage.pass_state, material.shader, VAO);
	dc.set_UBO(s_storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData), DrawCall::CopyData, 0);
	if(material.ubo.index != k_invalid_handle && material.data)
		dc.set_UBO(material.ubo, material.data, material.data_size, DrawCall::CopyData, 1);
	if(material.texture_group.index != k_invalid_handle)
	{
		const TextureGroup& tg = AssetManager::get(material.texture_group);
		for(uint32_t ii=0; ii<tg.texture_count; ++ii)
			dc.set_texture(tg.textures[ii], ii);
	}
	dc.set_key_depth(depth);
	Renderer::submit(dc);

	++s_storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return s_storage.num_draw_calls;
}


} // namespace erwin