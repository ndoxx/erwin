#include "render/renderer_pp.h"
#include "render/common_geometry.h"
#include "render/main_renderer.h"

namespace erwin
{

struct PPStorage
{
	UniformBufferHandle pp_ubo;

	ShaderHandle passthrough_shader;
	ShaderHandle pp_shader;
	ShaderHandle lighten_shader;
	PostProcessingData pp_data;

	uint32_t sequence = 0;
};
static PPStorage storage;

void PostProcessingRenderer::init()
{
    W_PROFILE_FUNCTION()

	storage.pp_ubo  = MainRenderer::create_uniform_buffer("post_proc_layout", nullptr, sizeof(PostProcessingData), DrawMode::Dynamic);
	// storage.passthrough_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/passthrough.glsl", "passthrough");
	storage.passthrough_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/passthrough.spv", "passthrough");
	// storage.pp_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc.glsl", "post_processing");
	storage.pp_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc.spv", "post_processing");
	
	storage.lighten_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc_lighten.glsl", "post_proc_lighten");

	MainRenderer::shader_attach_uniform_buffer(storage.pp_shader, storage.pp_ubo);
}

void PostProcessingRenderer::shutdown()
{
    W_PROFILE_FUNCTION()

	MainRenderer::destroy(storage.lighten_shader);
	MainRenderer::destroy(storage.pp_shader);
	MainRenderer::destroy(storage.passthrough_shader);
	MainRenderer::destroy(storage.pp_ubo);
}

void PostProcessingRenderer::begin_pass(const PostProcessingData& pp_data)
{
    W_PROFILE_FUNCTION()

	// Set post processing data
	storage.pp_data = pp_data;
	storage.sequence = 0;
}

void PostProcessingRenderer::blit(hash_t framebuffer, uint32_t index)
{
    W_PROFILE_FUNCTION()
	storage.pp_data.fb_size = FramebufferPool::get_size(framebuffer);
    
	PassState state;
	state.render_target = MainRenderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = false;
	state.rasterizer_state.clear_color = glm::vec4(0.0f,0.0f,0.0f,0.f);

	MainRenderer::get_queue("Presentation"_h).set_clear_color(state.rasterizer_state.clear_color); // TMP

	static DrawCall dc(DrawCall::Indexed, state.encode(), storage.pp_shader, CommonGeometry::get_vertex_array("quad"_h));
	dc.set_texture(MainRenderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	if(storage.pp_data.get_flag(PP_EN_BLOOM))
		dc.set_texture(MainRenderer::get_framebuffer_texture(FramebufferPool::get_framebuffer("bloom_combine"_h), 0), 1);
	dc.set_UBO(storage.pp_ubo, &storage.pp_data, sizeof(PostProcessingData), DrawCall::CopyData);
	dc.set_key_sequence(storage.sequence++, 0);
	MainRenderer::submit("Presentation"_h, dc);
}

void PostProcessingRenderer::lighten(hash_t framebuffer, uint32_t index)
{
    W_PROFILE_FUNCTION()
    
	PassState state;
	state.render_target = MainRenderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Light;
	state.depth_stencil_state.depth_test_enabled = false;
	state.rasterizer_state.clear_color = glm::vec4(0.0f,0.0f,0.0f,0.f);

	MainRenderer::get_queue("Presentation"_h).set_clear_color(state.rasterizer_state.clear_color); // TMP

	static DrawCall dc(DrawCall::Indexed, state.encode(), storage.lighten_shader, CommonGeometry::get_vertex_array("quad"_h));
	dc.set_texture(MainRenderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	dc.set_key_sequence(storage.sequence++, 0);
	MainRenderer::submit("Presentation"_h, dc);
}

void PostProcessingRenderer::end_pass()
{

}


} // namespace erwin