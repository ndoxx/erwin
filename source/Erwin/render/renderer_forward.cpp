#include "render/renderer_forward.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/global_ubos.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace erwin
{

struct LineInstanceData
{
	glm::mat4 mvp;
	glm::vec4 color;
};

static struct
{
	// Resources
	UniformBufferHandle line_ubo;
	ShaderHandle line_shader;

	// State
	uint64_t pass_state;
	uint8_t view_id;
} s_storage;

void ForwardRenderer::init()
{
    W_PROFILE_FUNCTION()

	s_storage.line_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/line_shader.glsl", "lines");

	// Setup UBOs and init storage
	s_storage.line_ubo = Renderer::create_uniform_buffer("line_data", nullptr, sizeof(LineInstanceData), UsagePattern::Dynamic);

	Renderer::shader_attach_uniform_buffer(s_storage.line_shader, s_storage.line_ubo);
}

void ForwardRenderer::shutdown()
{
	Renderer::destroy(s_storage.line_ubo);
	Renderer::destroy(s_storage.line_shader);
}

void ForwardRenderer::register_material(const Material& material)
{
	Renderer::shader_attach_uniform_buffer(material.shader, gu::get_frame_ubo());
	Renderer::shader_attach_uniform_buffer(material.shader, gu::get_transform_ubo());
	if(material.ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(material.shader, material.ubo);
}

void ForwardRenderer::begin_pass()
{
    W_PROFILE_FUNCTION()

	// Pass state
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = true;

	s_storage.pass_state = state.encode();
	s_storage.view_id = Renderer::next_layer_id();
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, const Material& material)
{
	// Compute matrices
	gu::TransformData transform_data;
	transform_data.m   = model_matrix;
	transform_data.mv  = gu::get_frame_data().view_matrix * transform_data.m;
	transform_data.mvp = gu::get_frame_data().view_projection_matrix * transform_data.m;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(transform_data.mvp, 3);
	float depth = clip.z/clip.w;
	SortKey key;
	key.set_depth(depth, s_storage.view_id, s_storage.pass_state, material.shader);

	DrawCall dc(DrawCall::Indexed, s_storage.pass_state, material.shader, VAO);
	dc.add_dependency(Renderer::update_uniform_buffer(gu::get_transform_ubo(), (void*)&transform_data, sizeof(gu::TransformData), DataOwnership::Copy));
	if(material.ubo.index != k_invalid_handle && material.data)
		dc.add_dependency(Renderer::update_uniform_buffer(material.ubo, material.data, material.data_size, DataOwnership::Copy));
	if(material.texture_group.index != k_invalid_handle)
	{
		const TextureGroup& tg = AssetManager::get(material.texture_group);
		for(uint32_t ii=0; ii<tg.texture_count; ++ii)
			dc.set_texture(tg.textures[ii], ii);
	}
	Renderer::submit(key.encode(), dc);
}

void ForwardRenderer::begin_line_pass(bool enable_depth_test)
{
	// State
	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = enable_depth_test;

	s_storage.pass_state = state.encode();
	s_storage.view_id = Renderer::next_layer_id();
}

void ForwardRenderer::end_line_pass()
{

}

void ForwardRenderer::draw_cube(const glm::mat4& model_matrix, glm::vec3 color)
{
	VertexArrayHandle VAO = CommonGeometry::get_vertex_array("cube_lines"_h);

	// Compute matrices
	LineInstanceData instance_data;
	instance_data.mvp = gu::get_frame_data().view_projection_matrix * model_matrix;
	instance_data.color = glm::vec4(color, 1.f);

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	SortKey key;
	key.set_depth(depth, s_storage.view_id, s_storage.pass_state, s_storage.line_shader);

	DrawCall dc(DrawCall::Indexed, s_storage.pass_state, s_storage.line_shader, VAO);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.line_ubo, (void*)&instance_data, sizeof(LineInstanceData), DataOwnership::Copy));
	Renderer::submit(key.encode(), dc);
}


} // namespace erwin