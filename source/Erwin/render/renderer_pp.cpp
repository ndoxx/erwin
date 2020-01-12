#include "render/renderer_pp.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "math/convolution.h"
#include "debug/texture_peek.h"

namespace erwin
{
// Do not actively send Gaussian kernel data in retail build, blur shader will use a fixed array of coefficients
#define BLOOM_RETAIL true
// Round bloom FBO dimensions to the next power of 2
#define BLOOM_FBO_NP2 false

constexpr uint32_t k_bloom_stage_count = 3;

#if BLOOM_RETAIL
struct BlurUBOData
{
	glm::vec2 offset;
};
#else
struct BlurUBOData
{
	glm::vec2 offset;
	int kernel_half_size;
	int padding = 0;
	float kernel_weights[math::k_max_kernel_coefficients];
};
#endif

static struct PPStorage
{
	PostProcessingData pp_data;

	UniformBufferHandle pp_ubo;
	UniformBufferHandle blur_ubo;
	ShaderHandle passthrough_shader;
	ShaderHandle pp_shader;
	ShaderHandle lighten_shader;
	ShaderHandle bloom_copy_shader;
	ShaderHandle bloom_blur_shader;
	ShaderHandle bloom_comb_shader;
	FramebufferHandle bloom_fbos[k_bloom_stage_count];
	FramebufferHandle bloom_tmp_fbos[k_bloom_stage_count];
	FramebufferHandle bloom_combine_fbo;
	float bloom_stage_ratios[k_bloom_stage_count];

#if !BLOOM_RETAIL
	math::SeparableGaussianKernel gk; // For bloom
#endif

	uint32_t sequence = 0;
} s_storage;

void PostProcessingRenderer::init()
{
    W_PROFILE_FUNCTION()

    // Create framebuffers for bloom pass
    {
	    FramebufferLayout layout =
	    {
	        {"color"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}
	    };
	    for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			std::string fb_name     = "bloom_" + std::to_string(ii);
			std::string fb_tmp_name = "bloom_tmp_" + std::to_string(ii);
			hash_t h_fb_name     = H_(fb_name.c_str());
			hash_t h_fb_tmp_name = H_(fb_tmp_name.c_str());
			float ratio = s_storage.bloom_stage_ratios[ii] = 1.f/(2.f+ii);//1.f/(pow(2.f,ii+1.f));
#if BLOOM_FBO_NP2
			s_storage.bloom_fbos[ii]     = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioNP2Constraint>(ratio, ratio), layout, false);
			s_storage.bloom_tmp_fbos[ii] = FramebufferPool::create_framebuffer(h_fb_tmp_name, make_scope<FbRatioNP2Constraint>(ratio, ratio), layout, false);
#else
			s_storage.bloom_fbos[ii]     = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(ratio, ratio), layout, false);
			s_storage.bloom_tmp_fbos[ii] = FramebufferPool::create_framebuffer(h_fb_tmp_name, make_scope<FbRatioConstraint>(ratio, ratio), layout, false);
#endif
			// TexturePeek::register_framebuffer(fb_name);
			// TexturePeek::register_framebuffer(fb_tmp_name);
		}

		// Bloom output framebuffer
		std::string fb_name = "bloom_combine";
		hash_t h_fb_name    = H_(fb_name.c_str());
		s_storage.bloom_combine_fbo = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(0.5f,0.5f), layout, false);
		TexturePeek::register_framebuffer(fb_name);
	}

	// Initialize Gaussian kernel for bloom blur passes
#if !BLOOM_RETAIL
	s_storage.gk.init(9,1.0f);
#endif

	// Create shaders
	s_storage.passthrough_shader = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/passthrough.spv", "passthrough");
	s_storage.pp_shader          = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc.spv", "post_processing");
	s_storage.lighten_shader     = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/post_proc_lighten.glsl", "post_proc_lighten");
	s_storage.bloom_copy_shader  = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_copy.glsl", "bloom_copy");
	s_storage.bloom_blur_shader  = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_blur.glsl", "bloom_blur");
	s_storage.bloom_comb_shader  = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_combine.glsl", "bloom_combine");
	
	s_storage.pp_ubo   = Renderer::create_uniform_buffer("post_proc_layout", nullptr, sizeof(PostProcessingData), DrawMode::Dynamic);
	s_storage.blur_ubo = Renderer::create_uniform_buffer("blur_data", nullptr, sizeof(BlurUBOData), DrawMode::Dynamic);

	Renderer::shader_attach_uniform_buffer(s_storage.pp_shader, s_storage.pp_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.bloom_blur_shader, s_storage.blur_ubo);

	// Reset sequence on end of frame
	Renderer::set_end_frame_callback([&](){ s_storage.sequence = 0; });
}

void PostProcessingRenderer::shutdown()
{
    W_PROFILE_FUNCTION()

	Renderer::destroy(s_storage.blur_ubo);
	Renderer::destroy(s_storage.pp_ubo);
	Renderer::destroy(s_storage.bloom_comb_shader);
	Renderer::destroy(s_storage.bloom_blur_shader);
	Renderer::destroy(s_storage.bloom_copy_shader);
	Renderer::destroy(s_storage.lighten_shader);
	Renderer::destroy(s_storage.pp_shader);
	Renderer::destroy(s_storage.passthrough_shader);
}

void PostProcessingRenderer::bloom_pass(hash_t source_fb, uint32_t glow_index, uint8_t layer_id)
{
	FramebufferHandle source_fb_handle = FramebufferPool::get_framebuffer(source_fb);

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

	PassState state;
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	// * For each bloom stage xx, render glow buffer to bloom_xx framebuffer
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			state.render_target = s_storage.bloom_fbos[ii];
			DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.bloom_copy_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(source_fb_handle, glow_index));
			dc.set_key_sequence(s_storage.sequence++);
			Renderer::submit(dc);
		}
	}

	BlurUBOData blur_data;
#if !BLOOM_RETAIL
	blur_data.kernel_half_size = s_storage.gk.half_size;
	memcpy(blur_data.kernel_weights, s_storage.gk.weights, math::k_max_kernel_coefficients);
#endif
	glm::vec2 screen_size = FramebufferPool::get_screen_size();

	// * For each bloom stage xx, given framebuffer bloom_xx as input,
	//   perform horizontal blur, output to bloom_tmp_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {1.f/target_size.x, 0.f}; // Offset is horizontal

			state.render_target = s_storage.bloom_tmp_fbos[ii];
			DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(s_storage.sequence++);
			Renderer::submit(dc);
		}
	}

	// * For each bloom stage xx, given framebuffer bloom_tmp_xx as input,
	//   perform vertical blur, output to bloom_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {0.f, 1.f/target_size.y}; // Offset is vertical

			state.render_target = s_storage.bloom_fbos[ii];
			DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(s_storage.bloom_tmp_fbos[ii], 0));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(s_storage.sequence++);
			Renderer::submit(dc);
		}
	}

	// * Combine each stage output to a single texture
	{
		state.render_target = s_storage.bloom_combine_fbo;
		DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.bloom_comb_shader, quad);
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
			dc.set_texture(Renderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0), ii);
		dc.set_key_sequence(s_storage.sequence++);
		Renderer::submit(dc);
	}
}

/*
	Alternative way to render bloom effect, inspired by what I did in WCore.
	No temporary FBO is used and bloom levels are combined using blending in lighten mode.
	The effect is less subtle (hence more prone to visible flickering) and takes about 
	the same time to render.
*/
void PostProcessingRenderer::bloom_pass_alt(hash_t source_fb, uint32_t glow_index, uint8_t layer_id)
{
	FramebufferHandle source_fb_handle = FramebufferPool::get_framebuffer(source_fb);

	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

	BlurUBOData blur_data;
#if !BLOOM_RETAIL
	blur_data.kernel_half_size = s_storage.gk.half_size;
	memcpy(blur_data.kernel_weights, s_storage.gk.weights, math::k_max_kernel_coefficients);
#endif
	glm::vec2 screen_size = FramebufferPool::get_screen_size();

	PassState state;
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;

	// * For each bloom stage xx, given glow buffer as input,
	//   perform horizontal blur, output to bloom_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {1.f/target_size.x, 0.f}; // Offset is horizontal

			state.render_target = s_storage.bloom_fbos[ii];
			DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(source_fb_handle, glow_index));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(s_storage.sequence++);
			Renderer::submit(dc);
		}
	}

	state.blend_state = BlendState::Light;
	state.render_target = s_storage.bloom_combine_fbo;
	uint64_t state_flags = state.encode();
	// * For each bloom stage xx, given framebuffer bloom_tmp_xx as input,
	//   perform vertical blur, output to bloom_combine
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {0.f, 1.f/target_size.y}; // Offset is vertical

			DrawCall dc(DrawCall::Indexed, layer_id, state_flags, s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(s_storage.sequence++);
			Renderer::submit(dc);
		}
	}
}

void PostProcessingRenderer::combine(hash_t framebuffer, uint32_t index, const PostProcessingData& pp_data, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()
	s_storage.pp_data = pp_data;
	s_storage.pp_data.fb_size = FramebufferPool::get_size(framebuffer);
    
	PassState state;
	state.render_target = Renderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = false;

	static DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.pp_shader, CommonGeometry::get_vertex_array("quad"_h));
	dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	if(s_storage.pp_data.get_flag(PP_EN_BLOOM))
		dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer("bloom_combine"_h), 0), 1);
	dc.set_UBO(s_storage.pp_ubo, &s_storage.pp_data, sizeof(PostProcessingData), DrawCall::CopyData);
	dc.set_key_sequence(s_storage.sequence++);
	Renderer::submit(dc);
}

void PostProcessingRenderer::lighten(hash_t framebuffer, uint32_t index, uint8_t layer_id)
{
    W_PROFILE_FUNCTION()
    
	PassState state;
	state.render_target = Renderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Light;
	state.depth_stencil_state.depth_test_enabled = false;

	static DrawCall dc(DrawCall::Indexed, layer_id, state.encode(), s_storage.lighten_shader, CommonGeometry::get_vertex_array("quad"_h));
	dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	dc.set_key_sequence(s_storage.sequence++);
	Renderer::submit(dc);
}


} // namespace erwin