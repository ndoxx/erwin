#include "render/renderer_forward.h"
#include "render/common_geometry.h"
#include "render/main_renderer.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "debug/texture_peek.h"

namespace erwin
{

constexpr uint32_t k_bloom_stage_count = 3;


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
	float light_ambient_strength;
};

struct InstanceData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

struct BlurUBOData
{
	glm::vec2 offset;
};

static struct ForwardRenderer3DStorage
{
	// Resources
	UniformBufferHandle instance_ubo;
	UniformBufferHandle pass_ubo;
	UniformBufferHandle blur_ubo;
	ShaderHandle bloom_copy_shader;
	ShaderHandle bloom_blur_shader;
	ShaderHandle bloom_comb_shader;
	FramebufferHandle forward_fbo;
	FramebufferHandle bloom_fbos[k_bloom_stage_count];
	FramebufferHandle bloom_tmp_fbos[k_bloom_stage_count];
	FramebufferHandle bloom_combine_fbo;
	float bloom_stage_ratios[k_bloom_stage_count];

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

    // Create framebuffer for forward pass
    {
	    FramebufferLayout layout =
	    {
	        {"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        {"glow"_h,   ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}, // For bloom effect
	    };
	    s_storage.forward_fbo = FramebufferPool::create_framebuffer("fb_forward"_h, make_scope<FbRatioConstraint>(), layout, true);
	}

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
			float ratio = s_storage.bloom_stage_ratios[ii] = 1.f/(pow(2.f,ii+1.f));
			s_storage.bloom_fbos[ii]     = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(ratio, ratio), layout, false);
			s_storage.bloom_tmp_fbos[ii] = FramebufferPool::create_framebuffer(h_fb_tmp_name, make_scope<FbRatioConstraint>(ratio, ratio), layout, false);
			// TexturePeek::register_framebuffer(fb_name);
			// TexturePeek::register_framebuffer(fb_tmp_name);
		}

		// Bloom output framebuffer
		std::string fb_name = "bloom_combine";
		hash_t h_fb_name    = H_(fb_name.c_str());
		s_storage.bloom_combine_fbo = FramebufferPool::create_framebuffer(h_fb_name, make_scope<FbRatioConstraint>(), layout, false);
		TexturePeek::register_framebuffer(fb_name);
	}

	// Create shaders
	s_storage.bloom_copy_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_copy.glsl", "bloom_copy");
	s_storage.bloom_blur_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_blur.glsl", "bloom_blur");
	s_storage.bloom_comb_shader = MainRenderer::create_shader(filesystem::get_system_asset_dir() / "shaders/bloom_combine.glsl", "bloom_combine");

	// Setup UBOs and init storage
	s_storage.instance_ubo = MainRenderer::create_uniform_buffer("instance_data", nullptr, sizeof(InstanceData), DrawMode::Dynamic);
	s_storage.pass_ubo     = MainRenderer::create_uniform_buffer("pass_data", nullptr, sizeof(PassUBOData), DrawMode::Dynamic);
	s_storage.blur_ubo     = MainRenderer::create_uniform_buffer("blur_data", nullptr, sizeof(BlurUBOData), DrawMode::Dynamic);
	s_storage.num_draw_calls = 0;
	
	MainRenderer::shader_attach_uniform_buffer(s_storage.bloom_blur_shader, s_storage.blur_ubo);
}

void ForwardRenderer::shutdown()
{
	MainRenderer::destroy(s_storage.instance_ubo);
	MainRenderer::destroy(s_storage.pass_ubo);
}

void ForwardRenderer::register_shader(ShaderHandle shader, UniformBufferHandle material_ubo)
{
	MainRenderer::shader_attach_uniform_buffer(shader, s_storage.pass_ubo);
	MainRenderer::shader_attach_uniform_buffer(shader, s_storage.instance_ubo);
	if(material_ubo.index != k_invalid_handle)
		MainRenderer::shader_attach_uniform_buffer(shader, material_ubo);
}

void ForwardRenderer::begin_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light, PassOptions options)
{
    W_PROFILE_FUNCTION()

	// Pass state
	PassState state;
	state.render_target = FramebufferPool::get_framebuffer("fb_forward"_h);
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = options.get_transparency() ? BlendState::Alpha : BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;
	state.rasterizer_state.clear_color = glm::vec4(0.0f,0.0f,0.0f,0.f);

	s_storage.pass_state = state.encode();
	s_storage.layer_id = options.get_layer_id();
	s_storage.draw_far = (options.get_depth_control() == PassOptions::DEPTH_CONTROL_FAR);

	// Reset stats
	s_storage.num_draw_calls = 0;

	// TMP
	MainRenderer::get_queue("ForwardOpaque"_h).set_clear_color(state.rasterizer_state.clear_color);

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
	s_storage.pass_ubo_data.light_ambient_strength = dir_light.ambient_strength;
	s_storage.frustum_planes = camera.get_frustum_planes();

	MainRenderer::update_uniform_buffer(s_storage.pass_ubo, &s_storage.pass_ubo_data, sizeof(PassUBOData));
}

void ForwardRenderer::end_pass()
{

}

void ForwardRenderer::bloom_pass()
{
	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);
	uint32_t sequence = 0;

	PassState state;
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.rasterizer_state.clear_color = glm::vec4(0.0f,0.0f,0.0f,0.f);
	MainRenderer::get_queue("Blur"_h).set_clear_color(state.rasterizer_state.clear_color);
	// * For each bloom stage xx, render glow buffer to bloom_xx framebuffer
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			state.render_target = s_storage.bloom_fbos[ii];
			DrawCall dc(DrawCall::Indexed, s_storage.bloom_copy_shader, quad);
			dc.set_state(state.encode());
			dc.set_texture(MainRenderer::get_framebuffer_texture(s_storage.forward_fbo, 1));
			dc.set_key_sequence(sequence++, s_storage.layer_id);
			MainRenderer::submit("Blur"_h, dc);
			++s_storage.num_draw_calls;
		}
	}

	BlurUBOData blur_data;
	float blur_offset_scale = 2.f;
	glm::vec2 screen_size = FramebufferPool::get_screen_size();

	// * For each bloom stage xx, given framebuffer bloom_xx as input,
	//   perform horizontal blur, output to bloom_tmp_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {blur_offset_scale/target_size.x, 0.f}; // Offset is horizontal

			state.render_target = s_storage.bloom_tmp_fbos[ii];
			DrawCall dc(DrawCall::Indexed, s_storage.bloom_blur_shader, quad);
			dc.set_state(state.encode());
			dc.set_texture(MainRenderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(sequence++, s_storage.layer_id);
			MainRenderer::submit("Blur"_h, dc);
			++s_storage.num_draw_calls;
		}
	}

	// * For each bloom stage xx, given framebuffer bloom_tmp_xx as input,
	//   perform vertical blur, output to bloom_xx
	{
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
		{
			glm::vec2 target_size = screen_size * s_storage.bloom_stage_ratios[ii];
			blur_data.offset = {0.f, blur_offset_scale/target_size.y}; // Offset is vertical

			state.render_target = s_storage.bloom_fbos[ii];
			DrawCall dc(DrawCall::Indexed, s_storage.bloom_blur_shader, quad);
			dc.set_state(state.encode());
			dc.set_texture(MainRenderer::get_framebuffer_texture(s_storage.bloom_tmp_fbos[ii], 0));
			dc.set_UBO(s_storage.blur_ubo, &blur_data, sizeof(BlurUBOData), DrawCall::CopyData, 0);
			dc.set_key_sequence(sequence++, s_storage.layer_id);
			MainRenderer::submit("Blur"_h, dc);
			++s_storage.num_draw_calls;
		}
	}

	// * Combine each stage output to a single texture
	{
		state.render_target = s_storage.bloom_combine_fbo;
		DrawCall dc(DrawCall::Indexed, s_storage.bloom_comb_shader, quad);
		dc.set_state(state.encode());
		for(uint32_t ii=0; ii<k_bloom_stage_count; ++ii)
			dc.set_texture(MainRenderer::get_framebuffer_texture(s_storage.bloom_fbos[ii], 0), ii);
		dc.set_key_sequence(sequence++, s_storage.layer_id);
		MainRenderer::submit("Blur"_h, dc);
		++s_storage.num_draw_calls;
	}
}

void ForwardRenderer::draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material)
{
	W_ASSERT_FMT(VAO.is_valid(), "Invalid VertexArrayHandle of index %hu.", VAO.index);

	// Compute matrices
	InstanceData instance_data;
	instance_data.m   = transform.get_model_matrix();
	instance_data.mv  = s_storage.pass_ubo_data.view_matrix * instance_data.m;
	instance_data.mvp = s_storage.pass_ubo_data.view_projection_matrix * instance_data.m;

	// Compute clip depth for the sorting key
	glm::vec4 clip = glm::column(instance_data.mvp, 3);
	float depth = clip.z/clip.w;
	
	DrawCall dc(DrawCall::Indexed, material.shader, VAO);
	dc.set_state(s_storage.pass_state);
	dc.set_UBO(s_storage.instance_ubo, (void*)&instance_data, sizeof(InstanceData), DrawCall::CopyData, 0);
	if(material.ubo.index != k_invalid_handle && material.data)
	{
		dc.set_UBO(material.ubo, material.data, material.data_size, DrawCall::CopyData, 1);
	}
	if(material.texture_group.index != k_invalid_handle)
	{
		const TextureGroup& tg = AssetManager::get(material.texture_group);
		for(uint32_t ii=0; ii<tg.texture_count; ++ii)
			dc.set_texture(tg.textures[ii], ii);
	}

	dc.set_key_depth(depth, s_storage.layer_id);
	// MainRenderer::submit(PassState::is_transparent(s_storage.pass_state) ? "ForwardTransparent"_h : "ForwardOpaque"_h, dc);
	MainRenderer::submit("ForwardOpaque"_h, dc);

	++s_storage.num_draw_calls;
}

uint32_t ForwardRenderer::get_draw_call_count()
{
	return s_storage.num_draw_calls;
}


} // namespace erwin