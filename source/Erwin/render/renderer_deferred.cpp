#include "render/renderer_deferred.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "render/common_geometry.h"
#include "render/global_ubos.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace erwin
{

static struct
{
	ShaderHandle dirlight_shader;

	uint64_t pass_state;
	uint8_t view_id;
} s_storage;

void DeferredRenderer::init()
{
	s_storage.dirlight_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/dir_light_deferred_PBR.glsl", "dir_light_deferred_PBR");
	Renderer::shader_attach_uniform_buffer(s_storage.dirlight_shader, gu::get_transform_ubo());
}

void DeferredRenderer::shutdown()
{
	Renderer::destroy(s_storage.dirlight_shader);
}

void DeferredRenderer::register_material(const Material& material)
{
	Renderer::shader_attach_uniform_buffer(material.shader, gu::get_frame_ubo());
	Renderer::shader_attach_uniform_buffer(material.shader, gu::get_transform_ubo());
	if(material.ubo.index != k_invalid_handle)
		Renderer::shader_attach_uniform_buffer(material.shader, material.ubo);
}

void DeferredRenderer::begin_pass()
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
	s_storage.view_id = Renderer::next_layer_id();
}

void DeferredRenderer::end_pass()
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
	uint8_t view_id = Renderer::next_layer_id();
	SortKey key;
	key.set_sequence(0, view_id, s_storage.dirlight_shader);

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.dirlight_shader, quad);
	for(int ii=0; ii<4; ++ii)
		dc.set_texture(Renderer::get_framebuffer_texture(GBuffer, ii), ii);
	Renderer::submit(key.encode(), dc);

	// Blit GBuffer's depth buffer into LBuffer
	key.set_sequence(1, view_id, s_storage.dirlight_shader);
	Renderer::blit_depth(key.encode(), GBuffer, LBuffer);
}

void DeferredRenderer::draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, const Material& material)
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




} // namespace erwin