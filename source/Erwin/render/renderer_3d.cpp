#include "render/renderer_3d.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/camera_3d.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/euler_angles.hpp"

#include <map>

namespace erwin
{

struct LineInstanceData
{
	glm::mat4 mvp;
	glm::vec4 color;
};

struct FrameData
{
	glm::mat4 view_matrix;
	glm::mat4 view_projection_matrix;
	glm::mat4 axis_aligned_view_projection_matrix;
	glm::vec4 eye_position;
	glm::vec4 camera_params;
	glm::vec4 framebuffer_size; // x,y: framebuffer dimensions in pixels, z: aspect ratio, w: padding
	glm::vec4 proj_params;
	
	glm::vec4 light_position;
	glm::vec4 light_color;
	glm::vec4 light_ambient_color;
	float light_ambient_strength;
};

struct TransformData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

struct Environment
{
	CubemapHandle cubemap;
};

static struct
{
	// Resources
	UniformBufferHandle line_ubo;
	ShaderHandle line_shader;
	ShaderHandle dirlight_shader;
	UniformBufferHandle frame_ubo;
	UniformBufferHandle transform_ubo;

	FrameData frame_data;
	Environment environment;

	std::map<uint32_t, int> registered_shaders;

	// State
	uint64_t pass_state;
	uint8_t layer_id;
} s_storage;

void Renderer3D::init()
{
    W_PROFILE_FUNCTION()

    // Init resources
	s_storage.line_shader     = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/line_shader.glsl", "lines");
	s_storage.dirlight_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/dir_light_deferred_PBR.glsl", "dir_light_deferred_PBR");
	s_storage.line_ubo        = Renderer::create_uniform_buffer("line_data", nullptr, sizeof(LineInstanceData), UsagePattern::Dynamic);
	s_storage.frame_ubo       = Renderer::create_uniform_buffer("frame_data", nullptr, sizeof(FrameData), UsagePattern::Dynamic);
	s_storage.transform_ubo   = Renderer::create_uniform_buffer("transform_data", nullptr, sizeof(TransformData), UsagePattern::Dynamic);

	Renderer::shader_attach_uniform_buffer(s_storage.dirlight_shader, s_storage.transform_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.line_shader, s_storage.line_ubo);
}

void Renderer3D::shutdown()
{
	Renderer::destroy(s_storage.transform_ubo);
	Renderer::destroy(s_storage.frame_ubo);
	Renderer::destroy(s_storage.line_ubo);
	Renderer::destroy(s_storage.dirlight_shader);
	Renderer::destroy(s_storage.line_shader);
}

void Renderer3D::update_frame_data(const PerspectiveCamera3D& camera, const ComponentDirectionalLight& dir_light)
{
	glm::vec2 fb_size = FramebufferPool::get_screen_size();
	float near = camera.get_frustum().near;
	float far  = camera.get_frustum().far;

	// Create a view matrix without the translational part, for skybox-type rendering
	glm::mat4 aa_view = camera.get_view_matrix();
	aa_view[3][0] = 0.f;
	aa_view[3][1] = 0.f;
	aa_view[3][2] = 0.f;

	s_storage.frame_data.view_matrix = camera.get_view_matrix();
	s_storage.frame_data.view_projection_matrix = camera.get_view_projection_matrix();
	s_storage.frame_data.axis_aligned_view_projection_matrix = camera.get_projection_matrix() * aa_view;

	s_storage.frame_data.eye_position = glm::vec4(camera.get_position(), 1.f);
	s_storage.frame_data.camera_params = glm::vec4(near,far,0.f,0.f);
	s_storage.frame_data.framebuffer_size = glm::vec4(fb_size, fb_size.x/fb_size.y, 0.f);
	s_storage.frame_data.proj_params = camera.get_projection_parameters();

	s_storage.frame_data.light_position = glm::vec4(dir_light.position, 0.f);
	s_storage.frame_data.light_color = glm::vec4(dir_light.color, 1.f) * dir_light.brightness;
	s_storage.frame_data.light_ambient_color = glm::vec4(dir_light.ambient_color, 1.f);
	s_storage.frame_data.light_ambient_strength = dir_light.ambient_strength;

	Renderer::update_uniform_buffer(s_storage.frame_ubo, &s_storage.frame_data, sizeof(FrameData));
}

void Renderer3D::set_environment_cubemap(CubemapHandle cubemap)
{
	s_storage.environment.cubemap = cubemap;
}

void Renderer3D::register_shader(MaterialHandle handle)
{
	const Material& material = AssetManager::get(handle);
	register_shader(material.shader, material.ubo, material.shader_flags);
}

void Renderer3D::register_shader(ShaderHandle shader, UniformBufferHandle ubo, int shader_flags)
{
	// Check if already registered
	if(s_storage.registered_shaders.find(shader.index) != s_storage.registered_shaders.end())
		return;

	Renderer::shader_attach_uniform_buffer(shader, s_storage.frame_ubo);
	Renderer::shader_attach_uniform_buffer(shader, s_storage.transform_ubo);
	if(ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(shader, ubo);

	s_storage.registered_shaders.insert({shader.index, shader_flags});
}

bool Renderer3D::is_compatible(VertexBufferLayoutHandle layout, MaterialHandle material)
{
	auto shader = AssetManager::get(material).shader;
	return Renderer::is_compatible(layout, shader);
}

void Renderer3D::begin_deferred_pass()
{
    W_PROFILE_FUNCTION()

	// Pass state
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("GBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	s_storage.pass_state = state.encode();
	s_storage.layer_id = Renderer::next_layer_id();
}

void Renderer3D::end_deferred_pass()
{
    /*
		TODO:
			[ ] SSAO pass
			[ ] SSR pass
    */

	FramebufferHandle GBuffer = FramebufferPool::get_framebuffer("GBuffer"_h);
	FramebufferHandle LBuffer = FramebufferPool::get_framebuffer("LBuffer"_h);

	// Directional light pass
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.depth_stencil_state.depth_lock = true;

	uint64_t state_flags = state.encode();
	uint8_t layer_id = Renderer::next_layer_id();
	SortKey key;
	key.set_sequence(0, layer_id, s_storage.dirlight_shader);

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.dirlight_shader, quad);
	for(int ii=0; ii<4; ++ii)
		dc.set_texture(Renderer::get_framebuffer_texture(GBuffer, ii), ii);
	Renderer::submit(key.encode(), dc);

	// Blit GBuffer's depth buffer into LBuffer
	key.set_sequence(1, layer_id, s_storage.dirlight_shader);
	Renderer::blit_depth(key.encode(), GBuffer, LBuffer);
}

void Renderer3D::begin_forward_pass(BlendState blend_state)
{
    W_PROFILE_FUNCTION()

	// Pass state
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = blend_state;
	state.depth_stencil_state.depth_test_enabled = true;

	s_storage.pass_state = state.encode();
	s_storage.layer_id = Renderer::next_layer_id();
}

void Renderer3D::end_forward_pass()
{

}

void Renderer3D::begin_line_pass(bool enable_depth_test)
{
	// State
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = enable_depth_test;

	s_storage.pass_state = state.encode();
	s_storage.layer_id = Renderer::next_layer_id();
}

void Renderer3D::end_line_pass()
{

}

void Renderer3D::draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, 
						   ShaderHandle shader, TextureGroupHandle texture_group, 
						   UniformBufferHandle ubo, void* material_data, uint32_t data_size)
{
	// Compute matrices
	TransformData transform_data;
	transform_data.m   = model_matrix;
	transform_data.mv  = s_storage.frame_data.view_matrix * transform_data.m;
	transform_data.mvp = s_storage.frame_data.view_projection_matrix * transform_data.m;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(transform_data.mvp, 3);
	float depth = clip.z/clip.w;
	SortKey key;
	key.set_depth(depth, s_storage.layer_id, s_storage.pass_state, shader);

	DrawCall dc(DrawCall::Indexed, s_storage.pass_state, shader, VAO);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.transform_ubo, (void*)&transform_data, sizeof(TransformData), DataOwnership::Copy));
	if(ubo.index != k_invalid_handle && material_data)
		dc.add_dependency(Renderer::update_uniform_buffer(ubo, material_data, data_size, DataOwnership::Copy));
	if(texture_group.index != k_invalid_handle)
	{
		const TextureGroup& tg = AssetManager::get(texture_group);
		for(uint32_t ii=0; ii<tg.texture_count; ++ii)
			dc.set_texture(tg.textures[ii], ii);
	}

	// Environment
	/*auto it = s_storage.registered_shaders.find(shader.index);
	if(it!=s_storage.registered_shaders.end())
	{
		int flags = it->second;
		if(flags & ShaderFlags::SAMPLE_ENVIRONMENT)
			dc.set_cubemap(s_storage.environment.cubemap, 0);
	}*/

	Renderer::submit(key.encode(), dc);
}

void Renderer3D::draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, MaterialHandle material_handle, void* material_data)
{
	const Material& material = AssetManager::get(material_handle);
	draw_mesh(VAO, model_matrix, material.shader, material.texture_group, material.ubo, material_data, material.data_size);
}

void Renderer3D::draw_cube(const glm::mat4& model_matrix, glm::vec3 color)
{
	VertexArrayHandle VAO = CommonGeometry::get_vertex_array("cube_lines"_h);

	// Compute matrices
	LineInstanceData instance_data;
	instance_data.mvp = s_storage.frame_data.view_projection_matrix * model_matrix;
	instance_data.color = glm::vec4(color, 1.f);

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	SortKey key;
	key.set_depth(depth, s_storage.layer_id, s_storage.pass_state, s_storage.line_shader);

	DrawCall dc(DrawCall::Indexed, s_storage.pass_state, s_storage.line_shader, VAO);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.line_ubo, (void*)&instance_data, sizeof(LineInstanceData), DataOwnership::Copy));
	Renderer::submit(key.encode(), dc);
}


} // namespace erwin