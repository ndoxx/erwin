#include "render/renderer_deferred.h"
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
};

struct InstanceData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

static struct
{
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;
	PassUBOData pass_ubo_data;

	uint64_t pass_state;
	uint8_t layer_id;
} s_storage;

void DeferredRenderer::init()
{
	s_storage.instance_ubo = Renderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	s_storage.pass_ubo     = Renderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
}

void DeferredRenderer::shutdown()
{
	Renderer::destroy(s_storage.pass_ubo);
	Renderer::destroy(s_storage.instance_ubo);
}

void DeferredRenderer::register_shader(ShaderHandle shader, UniformBufferHandle material_ubo)
{
	Renderer::shader_attach_uniform_buffer(shader, s_storage.pass_ubo);
	Renderer::shader_attach_uniform_buffer(shader, s_storage.instance_ubo);
	if(material_ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(shader, material_ubo);
}

void DeferredRenderer::begin_geometry_pass(const PerspectiveCamera3D& camera, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()

	// Pass state
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("GBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	s_storage.pass_state = state.encode();
	s_storage.layer_id = layer_id;

	// Set scene data
	glm::vec2 fb_size = FramebufferPool::get_screen_size();
	float near = camera.get_frustum().near;
	float far  = camera.get_frustum().far;

	s_storage.pass_ubo_data.view_matrix = camera.get_view_matrix();
	s_storage.pass_ubo_data.view_projection_matrix = camera.get_view_projection_matrix();
	s_storage.pass_ubo_data.eye_position = glm::vec4(camera.get_position(), 1.f);
	s_storage.pass_ubo_data.camera_params = glm::vec4(near,far,0.f,0.f);
	s_storage.pass_ubo_data.framebuffer_size = glm::vec4(fb_size, fb_size.x/fb_size.y, 0.f);

	Renderer::update_uniform_buffer(s_storage.pass_ubo, &s_storage.pass_ubo_data, sizeof(PassUBOData));
}

void DeferredRenderer::end_geometry_pass()
{

}

void DeferredRenderer::begin_light_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light)
{

}

void DeferredRenderer::end_light_pass()
{

}

void DeferredRenderer::draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material)
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
}




} // namespace erwin