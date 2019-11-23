#include "render/renderer_pp.h"

namespace erwin
{

struct PPStorage
{
	IndexBufferHandle sq_ibo;
	VertexBufferLayoutHandle sq_vbl;
	VertexBufferHandle sq_vbo;
	VertexArrayHandle sq_va;

	UniformBufferHandle pp_ubo;

	ShaderHandle passthrough_shader;
	ShaderHandle pp_shader;
	FramebufferHandle raw2d_framebuffer;

	uint64_t state_flags;
	PostProcessingData pp_data;
};
static PPStorage storage;

void PostProcessingRenderer::init()
{
    W_PROFILE_FUNCTION()

	float sq_vdata[20] = 
	{
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,   0.0f, 1.0f
	};
	uint32_t index_data[6] = { 0, 1, 2, 2, 3, 0 };

	storage.sq_ibo = MainRenderer::create_index_buffer(index_data, 6, DrawPrimitive::Triangles);
	storage.sq_vbl = MainRenderer::create_vertex_buffer_layout({
			    				 			    	{"a_position"_h, ShaderDataType::Vec3},
								 			    	{"a_uv"_h,       ShaderDataType::Vec2},
								 			    });
	storage.sq_vbo = MainRenderer::create_vertex_buffer(storage.sq_vbl, sq_vdata, 20, DrawMode::Static);
	storage.sq_va = MainRenderer::create_vertex_array(storage.sq_vbo, storage.sq_ibo);

	storage.pp_ubo  = MainRenderer::create_uniform_buffer("post_proc_layout", nullptr, sizeof(PostProcessingData), DrawMode::Dynamic);

	// Get handle to Renderer2D's render target
	storage.raw2d_framebuffer = FramebufferPool::get_framebuffer("fb_2d_raw"_h);

	// storage.passthrough_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/passthrough.glsl", "passthrough");
	storage.passthrough_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/passthrough.spv", "passthrough");
	// storage.pp_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc.glsl", "post_processing");
	storage.pp_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc.spv", "post_processing");

	MainRenderer::shader_attach_uniform_buffer(storage.pp_shader, storage.pp_ubo);

	auto& q_presentation = MainRenderer::get_queue(1);
	q_presentation.set_render_target(MainRenderer::default_render_target());
}

void PostProcessingRenderer::shutdown()
{
    W_PROFILE_FUNCTION()

	MainRenderer::destroy(storage.pp_shader);
	MainRenderer::destroy(storage.passthrough_shader);
	MainRenderer::destroy(storage.pp_ubo);
	MainRenderer::destroy(storage.sq_va);
	MainRenderer::destroy(storage.sq_vbo);
	MainRenderer::destroy(storage.sq_vbl);
	MainRenderer::destroy(storage.sq_ibo);
}

void PostProcessingRenderer::begin_pass(const PassState& state, const PostProcessingData& pp_data)
{
    W_PROFILE_FUNCTION()

	storage.state_flags = state.encode();

	// Set post processing data
	storage.pp_data = pp_data;
	storage.pp_data.fb_size = {FramebufferPool::get_width("fb_2d_raw"_h),
				  	   		   FramebufferPool::get_height("fb_2d_raw"_h)};
}

void PostProcessingRenderer::end_pass()
{
    W_PROFILE_FUNCTION()
    
	// Display to screen
	auto& q_presentation = MainRenderer::get_queue(1);
	// DrawCall dc(q_presentation, DrawCall::Indexed, storage.passthrough_shader, storage.sq_va);
	DrawCall dc(q_presentation, DrawCall::Indexed, storage.pp_shader, storage.sq_va);
	dc.set_state(storage.state_flags);
	dc.set_texture("us_input"_h, MainRenderer::get_framebuffer_texture(storage.raw2d_framebuffer, 0));
	dc.set_per_instance_UBO(storage.pp_ubo, &storage.pp_data, sizeof(PostProcessingData));
	dc.submit();
}


} // namespace erwin