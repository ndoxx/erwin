#include "render/renderer_pp.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "imgui.h"
#include <kibble/math/convolution.h>

namespace erwin
{
// Do not actively send Gaussian kernel data in retail build, blur shader will use a fixed array of coefficients
#define BLOOM_RETAIL true
// Round bloom FBO dimensions to the next power of 2
#define BLOOM_FBO_NP2 false

constexpr uint32_t k_bloom_stage_count = 3;

enum PPFlags: uint8_t
{
	PP_EN_CHROMATIC_ABERRATION = 1,
	PP_EN_EXPOSURE_TONE_MAPPING = 2,
	PP_EN_VIBRANCE = 4,
	PP_EN_SATURATION = 8,
	PP_EN_CONTRAST = 16,
	PP_EN_GAMMA = 32,
	PP_EN_FXAA = 64,
	PP_EN_BLOOM = 128
};

// #pragma pack(push,1)
struct PostProcessingData
{
	void set_flag_enabled(PPFlags flag, bool value) { if(value) set_flag(flag); else clear_flag(flag); }
	void set_flag(PPFlags flag)   { flags |= flag; }
	void clear_flag(PPFlags flag) { flags &= ~flag; }
	bool get_flag(PPFlags flag)   { return (flags & flag); }

	glm::vec4 vib_balance = glm::vec4(0.5f); // Vibrance
	glm::vec4 cor_gamma = glm::vec4(1.f);    // Color correction
	float ca_shift = 0.f;                    // Chromatic aberration
	float ca_strength = 0.f;                 // Chromatic aberration
	float tm_exposure = 2.718f;              // Exposure tone mapping
	float vib_strength = 0.f;                // Vibrance
	float cor_saturation = 1.f;              // Color correction
	float cor_contrast = 1.f;                // Color correction
	
	// Filled in by renderer
	glm::vec2 fb_size;                       // Framebuffer size
	
	uint32_t flags = 0;						 // Flags to enable/disable post-processing features
};
// #pragma pack(pop)

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

static struct
{
	PostProcessingData pp_data;

	UniformBufferHandle pp_ubo;
	UniformBufferHandle blur_ubo;
	ShaderHandle passthrough_shader;
	ShaderHandle pp_shader;
	ShaderHandle lighten_shader;
	ShaderHandle bloom_blur_shader;
	FramebufferHandle final_render_target;
	FramebufferHandle bloom_fbos[k_bloom_stage_count];
	FramebufferHandle bloom_combine_fbo;
	float bloom_stage_ratios[k_bloom_stage_count];

#if !BLOOM_RETAIL
	kb::math::SeparableGaussianKernel gk; // For bloom
#endif

	uint32_t sequence = 0;
} s_storage;

void PostProcessingRenderer::init(EventBus& event_bus)
{
    W_PROFILE_FUNCTION()

    s_storage.final_render_target = Renderer::default_render_target();

    // Create framebuffers for bloom pass
    {
	    FramebufferLayout layout
	    {
	        {"color"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}
	    };
	    for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			std::string fb_name     = "bloom_" + std::to_string(ii);
			hash_t h_fb_name     = H_(fb_name.c_str());
			float ratio = s_storage.bloom_stage_ratios[ii] = 1.f/(2.f+float(ii));//1.f/(pow(2.f,ii+1.f));
#if BLOOM_FBO_NP2
			s_storage.bloom_fbos[ii] = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioNP2Constraint>(ratio, ratio), FB_NONE, layout);
#else
			s_storage.bloom_fbos[ii] = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(ratio, ratio), FB_NONE, layout);
#endif
		}

		// Bloom output framebuffer
		std::string fb_name = "BloomCombine";
		hash_t h_fb_name    = H_(fb_name.c_str());
		s_storage.bloom_combine_fbo = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(0.5f,0.5f), FB_NONE, layout);
	}

	// Initialize Gaussian kernel for bloom blur passes
#if !BLOOM_RETAIL
	s_storage.gk.init(9,1.0f);
#endif

	// Create shaders
	s_storage.passthrough_shader = Renderer::create_shader("sysres://shaders/passthrough.glsl", "passthrough");
	s_storage.pp_shader          = Renderer::create_shader("sysres://shaders/post_proc.glsl", "post_processing");
	s_storage.lighten_shader     = Renderer::create_shader("sysres://shaders/post_proc_lighten.glsl", "post_proc_lighten");
	s_storage.bloom_blur_shader  = Renderer::create_shader("sysres://shaders/bloom_blur.glsl", "bloom_blur");
	
	s_storage.pp_ubo   = Renderer::create_uniform_buffer("post_proc_layout", nullptr, sizeof(PostProcessingData), UsagePattern::Dynamic);
	s_storage.blur_ubo = Renderer::create_uniform_buffer("blur_data", nullptr, sizeof(BlurUBOData), UsagePattern::Dynamic);

	Renderer::shader_attach_uniform_buffer(s_storage.pp_shader, s_storage.pp_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.bloom_blur_shader, s_storage.blur_ubo);

	// Reset sequence on new frame
	event_bus.subscribe<BeginFrameEvent>([](const BeginFrameEvent&) -> bool
	{
		s_storage.sequence = 0;
		return false;
	});

    s_storage.pp_data.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_VIBRANCE, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_SATURATION, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_CONTRAST, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_GAMMA, true);
    s_storage.pp_data.set_flag_enabled(PP_EN_FXAA, true);
}

void PostProcessingRenderer::shutdown()
{
    W_PROFILE_FUNCTION()

	Renderer::destroy(s_storage.blur_ubo);
	Renderer::destroy(s_storage.pp_ubo);
	Renderer::destroy(s_storage.bloom_blur_shader);
	Renderer::destroy(s_storage.lighten_shader);
	Renderer::destroy(s_storage.pp_shader);
	Renderer::destroy(s_storage.passthrough_shader);
}

void PostProcessingRenderer::set_final_render_target(hash_t fb_hname)
{
	if(fb_hname == "default"_h || fb_hname == 0)
		s_storage.final_render_target = Renderer::default_render_target();
	else
		s_storage.final_render_target = FramebufferPool::get_framebuffer(fb_hname);
}

void PostProcessingRenderer::bloom_pass(hash_t source_fb, uint32_t glow_index)
{
	FramebufferHandle source_fb_handle = FramebufferPool::get_framebuffer(source_fb);

	VertexArrayHandle quad = CommonGeometry::get_mesh("quad"_h).VAO;

	BlurUBOData blur_data;
#if !BLOOM_RETAIL
	blur_data.kernel_half_size = s_storage.gk.half_size;
	memcpy(blur_data.kernel_weights, s_storage.gk.weights, math::k_max_kernel_coefficients);
#endif
	glm::vec2 screen_size = FramebufferPool::get_screen_size();

	uint8_t view_id = Renderer::next_layer_id();
	SortKey key;
	RenderState state;
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;

	// * For each bloom stage xx, given glow buffer as input,
	//   perform horizontal blur, output to bloom_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {1.f/target_size.x, 0.f}; // Offset is horizontal

			state.render_target = s_storage.bloom_fbos[ii].index();
			uint64_t state_flags = state.encode();
			key.set_sequence(s_storage.sequence++, view_id, s_storage.bloom_blur_shader);

			DrawCall dc(DrawCall::Indexed, state_flags, s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(source_fb_handle, glow_index));
			dc.add_dependency(Renderer::update_uniform_buffer(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DataOwnership::Copy));
			Renderer::submit(key.encode(), dc);
		}
	}

	state.blend_state = BlendState::Light;
	state.render_target = s_storage.bloom_combine_fbo.index();
	uint64_t state_flags = state.encode();
	// * For each bloom stage xx, given framebuffer bloom_tmp_xx as input,
	//   perform vertical blur, output to bloom_combine
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {0.f, 1.f/target_size.y}; // Offset is vertical

			key.set_sequence(s_storage.sequence++, view_id, s_storage.bloom_blur_shader);

			DrawCall dc(DrawCall::Indexed, state_flags, s_storage.bloom_blur_shader, quad);
			dc.set_texture(Renderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0));
			dc.add_dependency(Renderer::update_uniform_buffer(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DataOwnership::Copy));
			Renderer::submit(key.encode(), dc);
		}
	}
}

void PostProcessingRenderer::combine(hash_t framebuffer, uint32_t index, bool use_bloom)
{
    W_PROFILE_FUNCTION()
	s_storage.pp_data.fb_size = FramebufferPool::get_size(framebuffer);
	s_storage.pp_data.set_flag_enabled(PP_EN_BLOOM, use_bloom);
    
	uint8_t view_id = Renderer::next_layer_id();
	SortKey key;
	RenderState state;
	state.render_target = s_storage.final_render_target.index();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG;
	state.blend_state = BlendState::Alpha;
	state.depth_stencil_state.depth_test_enabled = false;

	key.set_sequence(s_storage.sequence++, view_id, s_storage.pp_shader);
	DrawCall dc(DrawCall::Indexed, state.encode(), s_storage.pp_shader, CommonGeometry::get_mesh("quad"_h).VAO);
	dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	if(use_bloom)
		dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer("BloomCombine"_h), 0), 1);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.pp_ubo, &s_storage.pp_data, sizeof(PostProcessingData), DataOwnership::Copy));
	Renderer::submit(key.encode(), dc);
}

void PostProcessingRenderer::lighten(hash_t framebuffer, uint32_t index)
{
    W_PROFILE_FUNCTION()
    
	uint8_t view_id = Renderer::next_layer_id();
	SortKey key;
	RenderState state;
	state.render_target = s_storage.final_render_target.index();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG;
	state.blend_state = BlendState::Light;
	state.depth_stencil_state.depth_test_enabled = false;

	key.set_sequence(s_storage.sequence++, view_id, s_storage.lighten_shader);
	DrawCall dc(DrawCall::Indexed, state.encode(), s_storage.lighten_shader, CommonGeometry::get_mesh("quad"_h).VAO);
	dc.set_texture(Renderer::get_framebuffer_texture(FramebufferPool::get_framebuffer(framebuffer), index));
	Renderer::submit(key.encode(), dc);
}

static bool s_enable_chromatic_aberration  = true;
static bool s_enable_exposure_tone_mapping = true;
static bool s_enable_saturation            = true;
static bool s_enable_contrast              = true;
static bool s_enable_gamma                 = true;
static bool s_enable_vibrance              = true;
void PostProcessingRenderer::on_imgui_render()
{
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Chromatic aberration"))
    {
		if(ImGui::Checkbox("Enable##en_ca", &s_enable_chromatic_aberration))
			s_storage.pp_data.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, s_enable_chromatic_aberration);

        ImGui::SliderFloat("Shift",     &s_storage.pp_data.ca_shift, 0.0f, 10.0f);
        ImGui::SliderFloat("Magnitude", &s_storage.pp_data.ca_strength, 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Tone mapping"))
    {
		if(ImGui::Checkbox("Enable##en_tm", &s_enable_exposure_tone_mapping))
			s_storage.pp_data.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, s_enable_exposure_tone_mapping);

        ImGui::SliderFloat("Exposure", &s_storage.pp_data.tm_exposure, 0.1f, 5.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Correction"))
    {
		if(ImGui::Checkbox("Enable##en_sat", &s_enable_saturation))
			s_storage.pp_data.set_flag_enabled(PP_EN_SATURATION, s_enable_saturation);

        ImGui::SliderFloat("Saturation", &s_storage.pp_data.cor_saturation, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_cnt", &s_enable_contrast))
			s_storage.pp_data.set_flag_enabled(PP_EN_CONTRAST, s_enable_contrast);

        ImGui::SliderFloat("Contrast", &s_storage.pp_data.cor_contrast, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_gam", &s_enable_gamma))
			s_storage.pp_data.set_flag_enabled(PP_EN_GAMMA, s_enable_gamma);

        ImGui::SliderFloat3("Gamma", &s_storage.pp_data.cor_gamma[0], 1.0f, 2.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Vibrance"))
    {
		if(ImGui::Checkbox("Enable##en_vib", &s_enable_vibrance))
			s_storage.pp_data.set_flag_enabled(PP_EN_VIBRANCE, s_enable_vibrance);

        ImGui::SliderFloat("Strength", &s_storage.pp_data.vib_strength, -1.0f, 2.0f);
        ImGui::SliderFloat3("Balance", &s_storage.pp_data.vib_balance[0], 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
}


} // namespace erwin