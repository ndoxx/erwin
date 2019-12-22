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

	uint64_t pass_state;
	PostProcessingData pp_data;
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

	MainRenderer::shader_attach_uniform_buffer(storage.pp_shader, storage.pp_ubo);
}

void PostProcessingRenderer::shutdown()
{
    W_PROFILE_FUNCTION()

	MainRenderer::destroy(storage.pp_shader);
	MainRenderer::destroy(storage.passthrough_shader);
	MainRenderer::destroy(storage.pp_ubo);
}

void PostProcessingRenderer::begin_pass(const PostProcessingData& pp_data)
{
    W_PROFILE_FUNCTION()

	PassState state;
	state.render_target = MainRenderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = false;
	state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

	storage.pass_state = state.encode();
	MainRenderer::get_queue("Presentation"_h).set_clear_color(state.rasterizer_state.clear_color); // TMP

	// Set post processing data
	storage.pp_data = pp_data;
}

void PostProcessingRenderer::blit(hash_t framebuffer, uint32_t index)
{
    W_PROFILE_FUNCTION()
	storage.pp_data.fb_size = FramebufferPool::get_size(framebuffer);
    
	static DrawCall dc(DrawCall::Indexed, storage.pp_shader, CommonGeometry::get_vertex_array("screen_quad"_h));
	dc.set_state(storage.pass_state);
	dc.set_texture(MainRenderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	dc.set_per_instance_UBO(storage.pp_ubo, &storage.pp_data, sizeof(PostProcessingData), DrawCall::CopyData);
	MainRenderer::submit("Presentation"_h, dc);
}

void PostProcessingRenderer::end_pass()
{

}


} // namespace erwin