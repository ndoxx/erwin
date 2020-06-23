#include "render/renderer_3d.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "asset/environment.h"
#include "asset/texture.h"
#include "asset/asset_manager.h"
#include "asset/material.h"
#include "asset/mesh.h"
#include "filesystem/filesystem.h"
#include "entity/component_camera.h"
#include "entity/light.h"
#include "math/transform.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/euler_angles.hpp"

#include <set>

namespace erwin
{

struct LineInstanceData
{
	glm::mat4 mvp;
	glm::vec4 color;
};

enum FrameDataFlags: int
{
	FD_NONE          = 0,
	FD_ENABLE_IBL    = 1<<0,
	FD_DEBUG_SHOW_UV = 1<<1,
};

struct FrameData
{
	glm::mat4 view_matrix;
	glm::mat4 transposed_view_matrix;
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
	int flags;
};

struct TransformData
{
	glm::mat4 m;
	glm::mat4 mv;
	glm::mat4 mvp;
};

struct EquirectangularConversionData
{
	glm::vec2 viewport_size;
};

struct DiffuseIrradianceData
{
	glm::vec2 viewport_size;
	float delta_sample;
};

struct PrefilterEnvmapData
{
	glm::vec2 viewport_size;
	float roughness;
	float source_resolution;
};

static struct
{
	// Resources
	ShaderHandle line_shader;
	ShaderHandle dirlight_shader;
	ShaderHandle skybox_shader;
	ShaderHandle equirectangular_to_cubemap_shader;
	ShaderHandle diffuse_irradiance_shader;
	ShaderHandle prefilter_env_map_shader;
	UniformBufferHandle line_ubo;
	UniformBufferHandle frame_ubo;
	UniformBufferHandle transform_ubo;
	UniformBufferHandle equirectangular_conversion_ubo;
	UniformBufferHandle diffuse_irradiance_ubo;
	UniformBufferHandle prefilter_env_map_ubo;
	TextureHandle BRDF_integration_map;

	FrameData frame_data;
	Environment environment;

	std::set<uint32_t> registered_shaders;

	// State
	uint64_t pass_state;
	uint8_t layer_id;
} s_storage;

void Renderer3D::init()
{
    W_PROFILE_FUNCTION()

    // Init resources
	s_storage.line_shader                       = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/line_shader.glsl", "lines");
	s_storage.dirlight_shader                   = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/deferred_PBR_lighting.glsl", "deferred_PBR_lighting");
	s_storage.skybox_shader                     = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/skybox.glsl", "skybox");
	s_storage.equirectangular_to_cubemap_shader = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/equirectangular_to_cubemap.glsl", "ER2C");
	s_storage.diffuse_irradiance_shader         = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/diffuse_irradiance.glsl", "diffuse_irradiance");
	s_storage.prefilter_env_map_shader          = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/prefilter_env_map.glsl", "prefilter_env_map");
	s_storage.line_ubo                          = Renderer::create_uniform_buffer("line_data", nullptr, sizeof(LineInstanceData), UsagePattern::Dynamic);
	s_storage.frame_ubo                         = Renderer::create_uniform_buffer("frame_data", nullptr, sizeof(FrameData), UsagePattern::Dynamic);
	s_storage.transform_ubo                     = Renderer::create_uniform_buffer("transform_data", nullptr, sizeof(TransformData), UsagePattern::Dynamic);
	s_storage.equirectangular_conversion_ubo    = Renderer::create_uniform_buffer("parameters", nullptr, sizeof(EquirectangularConversionData), UsagePattern::Dynamic);
	s_storage.diffuse_irradiance_ubo            = Renderer::create_uniform_buffer("parameters", nullptr, sizeof(DiffuseIrradianceData), UsagePattern::Dynamic);
	s_storage.prefilter_env_map_ubo             = Renderer::create_uniform_buffer("parameters", nullptr, sizeof(PrefilterEnvmapData), UsagePattern::Dynamic);

	Renderer::shader_attach_uniform_buffer(s_storage.dirlight_shader, s_storage.transform_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.line_shader, s_storage.line_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.skybox_shader, s_storage.frame_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.equirectangular_to_cubemap_shader, s_storage.equirectangular_conversion_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.diffuse_irradiance_shader, s_storage.diffuse_irradiance_ubo);
	Renderer::shader_attach_uniform_buffer(s_storage.prefilter_env_map_shader, s_storage.prefilter_env_map_ubo);

	// Load BRDF integration map
	Texture2DDescriptor brdf_lut_desc;
	brdf_lut_desc.image_format = ImageFormat::RGBA8;
	brdf_lut_desc.wrap = TextureWrap::CLAMP_TO_EDGE;
	brdf_lut_desc.filter = MIN_LINEAR | MAG_LINEAR;
	const auto& freetex = AssetManager::load_texture(wfs::get_system_asset_dir() / "textures/ibl_brdf_integration.png", brdf_lut_desc);
	s_storage.BRDF_integration_map = freetex.handle;
}

void Renderer3D::shutdown()
{
	Renderer::destroy(s_storage.BRDF_integration_map);
	Renderer::destroy(s_storage.prefilter_env_map_ubo);
	Renderer::destroy(s_storage.diffuse_irradiance_ubo);
	Renderer::destroy(s_storage.equirectangular_conversion_ubo);
	Renderer::destroy(s_storage.transform_ubo);
	Renderer::destroy(s_storage.frame_ubo);
	Renderer::destroy(s_storage.line_ubo);
	Renderer::destroy(s_storage.equirectangular_to_cubemap_shader);
	Renderer::destroy(s_storage.diffuse_irradiance_shader);
	Renderer::destroy(s_storage.prefilter_env_map_shader);
	Renderer::destroy(s_storage.skybox_shader);
	Renderer::destroy(s_storage.dirlight_shader);
	Renderer::destroy(s_storage.line_shader);
}

void Renderer3D::update_camera(const ComponentCamera3D& camera, const Transform3D& transform)
{
	glm::vec2 fb_size = FramebufferPool::get_screen_size();
	float near = camera.frustum.near;
	float far  = camera.frustum.far;

	// Create a view matrix without the translational part, for skybox-type rendering
	glm::mat4 aa_view = camera.view_matrix;
	aa_view[3][0] = 0.f;
	aa_view[3][1] = 0.f;
	aa_view[3][2] = 0.f;

	s_storage.frame_data.view_matrix = camera.view_matrix;
	s_storage.frame_data.transposed_view_matrix = glm::transpose(camera.view_matrix);
	s_storage.frame_data.view_projection_matrix = camera.view_projection_matrix;
	s_storage.frame_data.axis_aligned_view_projection_matrix = camera.projection_matrix * aa_view;

	s_storage.frame_data.eye_position = glm::vec4(transform.position, 1.f);
	s_storage.frame_data.camera_params = glm::vec4(near,far,0.f,0.f);
	s_storage.frame_data.framebuffer_size = glm::vec4(fb_size, fb_size.x/fb_size.y, 0.f);
	s_storage.frame_data.proj_params = camera.projection_parameters;
}

void Renderer3D::update_light(const ComponentDirectionalLight& dir_light)
{
	s_storage.frame_data.light_position = glm::vec4(dir_light.position, 0.f);
	s_storage.frame_data.light_color = glm::vec4(dir_light.color, 1.f) * dir_light.brightness;
	s_storage.frame_data.light_ambient_color = glm::vec4(dir_light.ambient_color, 1.f);

	// Environment
	if(s_storage.environment.IBL_enabled)
	{
		s_storage.frame_data.flags |= FrameDataFlags::FD_ENABLE_IBL;
		s_storage.frame_data.light_ambient_strength = s_storage.environment.ambient_strength;
	}
	else
	{
		s_storage.frame_data.flags &= ~FrameDataFlags::FD_ENABLE_IBL;
		s_storage.frame_data.light_ambient_strength = dir_light.ambient_strength;
	}
}

void Renderer3D::update_frame_data()
{
	Renderer::update_uniform_buffer(s_storage.frame_ubo, &s_storage.frame_data, sizeof(FrameData));
}

void Renderer3D::set_environment(const Environment& environment)
{
	s_storage.environment = environment;
}

void Renderer3D::enable_IBL(bool value)
{
	s_storage.environment.IBL_enabled = value;
}

void Renderer3D::set_IBL_ambient_strength(float value)
{
	s_storage.environment.ambient_strength = value;
}

void Renderer3D::register_shader(ShaderHandle shader)
{
	// Check if already registered
	if(s_storage.registered_shaders.find(shader.index) != s_storage.registered_shaders.end())
		return;

	Renderer::shader_attach_uniform_buffer(shader, s_storage.frame_ubo);
	Renderer::shader_attach_uniform_buffer(shader, s_storage.transform_ubo);

	s_storage.registered_shaders.insert(shader.index);
}
/*
bool Renderer3D::is_compatible(VertexBufferLayoutHandle layout, const Material& material)
{
	return Renderer::is_compatible(layout, material.shader);
}
*/
void Renderer3D::debug_show_uv(bool enabled)
{
	if(enabled)
		s_storage.frame_data.flags |= FrameDataFlags::FD_DEBUG_SHOW_UV;
	else
		s_storage.frame_data.flags &= ~FrameDataFlags::FD_DEBUG_SHOW_UV;
}





CubemapHandle Renderer3D::generate_cubemap_hdr(TextureHandle hdr_tex, uint32_t size)
{
	// Create an ad-hoc framebuffer to render to a cubemap
	FramebufferLayout layout
	{
	    {"cubemap"_h, ImageFormat::RGB16F, MIN_LINEAR_MIPMAP_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE, 5, true}
	};
	FramebufferHandle fb = Renderer::create_framebuffer(size, size, FB_CUBEMAP_ATTACHMENT, layout);
	CubemapHandle cubemap = Renderer::get_framebuffer_cubemap(fb);

	// Draw call
	EquirectangularConversionData data;
	data.viewport_size = { size, size };

	// Render a single quad, the geometry shader will perform layered rendering with 6 invocations
	RenderState state;
	state.render_target = fb;
	state.rasterizer_state.cull_mode = CullMode::None;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.depth_stencil_state.depth_lock = true;

	uint64_t state_flags = state.encode();

	// TODO: Reserve several layer IDs for this kind of operations that
	// only take place once in a while
	SortKey key;
	key.set_sequence(0, 0, s_storage.equirectangular_to_cubemap_shader);

	VertexArrayHandle quad = CommonGeometry::get_mesh("quad"_h).VAO;
	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.equirectangular_to_cubemap_shader, quad);
	dc.set_texture(hdr_tex);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.equirectangular_conversion_ubo, static_cast<void*>(&data), sizeof(EquirectangularConversionData), DataOwnership::Copy));

	Renderer::submit(key.encode(), dc);
	Renderer::generate_mipmaps(cubemap);

	// Cleanup
	Renderer::destroy(fb, true); // Destroy FB but keep cubemap attachment alive

	return cubemap;
}

CubemapHandle Renderer3D::generate_irradiance_map(CubemapHandle env_map)
{
	constexpr uint32_t ecm_size = 32;

	// Create an ad-hoc framebuffer to render to a cubemap
	FramebufferLayout layout
	{
	    {"cubemap"_h, ImageFormat::RGB16F, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}
	};
	FramebufferHandle fb = Renderer::create_framebuffer(ecm_size, ecm_size, FB_CUBEMAP_ATTACHMENT, layout);
	CubemapHandle ecm = Renderer::get_framebuffer_cubemap(fb);

	DiffuseIrradianceData data;
	data.viewport_size = {ecm_size, ecm_size};
	data.delta_sample = 0.025f;

	// Render a single quad, the geometry shader will perform layered rendering with 6 invocations
	RenderState state;
	state.render_target = fb;
	state.rasterizer_state.cull_mode = CullMode::None;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.depth_stencil_state.depth_lock = true;

	uint64_t state_flags = state.encode();

	SortKey key;
	key.set_sequence(1, 0, s_storage.diffuse_irradiance_shader);

	VertexArrayHandle quad = CommonGeometry::get_mesh("quad"_h).VAO;
	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.diffuse_irradiance_shader, quad);
	dc.set_cubemap(env_map);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.diffuse_irradiance_ubo, static_cast<void*>(&data), sizeof(DiffuseIrradianceData), DataOwnership::Copy));

	Renderer::submit(key.encode(), dc);

	// Cleanup
	Renderer::destroy(fb, true); // Destroy FB but keep cubemap attachment alive

	return ecm;
}

CubemapHandle Renderer3D::generate_prefiltered_map(CubemapHandle env_map, uint32_t source_resolution)
{
	constexpr uint32_t pfm_size = 512;
	constexpr uint8_t max_mips = 5;

	// Create an ad-hoc framebuffer to render to a cubemap
	FramebufferLayout layout
	{
	    {"cubemap"_h, ImageFormat::RGB16F, MIN_LINEAR_MIPMAP_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE, max_mips-1}
	};
	FramebufferHandle fb = Renderer::create_framebuffer(pfm_size, pfm_size, FB_CUBEMAP_ATTACHMENT, layout);
	CubemapHandle pfm = Renderer::get_framebuffer_cubemap(fb);

	PrefilterEnvmapData data;
	data.source_resolution = float(source_resolution);

	// Render a single quad, the geometry shader will perform layered rendering with 6 invocations
	RenderState state;
	state.render_target = fb;
	state.rasterizer_state.cull_mode = CullMode::None;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = false;
	state.depth_stencil_state.depth_lock = true;

	for(uint8_t mip_level=0; mip_level<max_mips; ++mip_level)
	{
        uint32_t mip_size  = std::max(1u, uint32_t(pfm_size / (1u << mip_level)));
		data.viewport_size = {mip_size, mip_size};
		data.roughness = float(mip_level) / float(max_mips - 1);
		state.target_mip_level = mip_level;

		uint64_t state_flags = state.encode();

		SortKey key;
		key.set_sequence(uint32_t(2+mip_level), 0, s_storage.prefilter_env_map_shader);

		VertexArrayHandle quad = CommonGeometry::get_mesh("quad"_h).VAO;
		DrawCall dc(DrawCall::Indexed, state_flags, s_storage.prefilter_env_map_shader, quad);
		dc.set_cubemap(env_map);
		dc.add_dependency(Renderer::update_uniform_buffer(s_storage.prefilter_env_map_ubo, static_cast<void*>(&data), sizeof(PrefilterEnvmapData), DataOwnership::Copy));

		Renderer::submit(key.encode(), dc);
	}

	// Cleanup
	Renderer::destroy(fb, true); // Destroy FB but keep cubemap attachment alive

	return pfm;
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

	VertexArrayHandle quad = CommonGeometry::get_mesh("quad"_h).VAO;
	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.dirlight_shader, quad);
	for(uint32_t ii=0; ii<4; ++ii)
		dc.set_texture(Renderer::get_framebuffer_texture(GBuffer, ii), ii);
	// IBL
	if(s_storage.environment.diffuse_irradiance_map.is_valid())
	{
		dc.set_texture(s_storage.BRDF_integration_map, 4);
		dc.set_cubemap(s_storage.environment.diffuse_irradiance_map, 0);
		dc.set_cubemap(s_storage.environment.prefiltered_map, 1);
	}

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
	state.rasterizer_state.clear_flags = CLEAR_DEPTH_FLAG;
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

void Renderer3D::draw_mesh(const Mesh& mesh, const glm::mat4& model_matrix, const Material& material, const void* material_data)
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
	key.set_depth(depth, s_storage.layer_id, s_storage.pass_state, material.shader);

	DrawCall dc(DrawCall::Indexed, s_storage.pass_state, material.shader, mesh.VAO);
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.transform_ubo, static_cast<void*>(&transform_data), sizeof(TransformData), DataOwnership::Copy));
	if(material.ubo.index != k_invalid_handle && material_data)
		dc.add_dependency(Renderer::update_uniform_buffer(material.ubo, material_data, material.data_size, DataOwnership::Copy));
	for(uint32_t ii=0; ii<material.texture_group.texture_count; ++ii)
		dc.set_texture(material.texture_group.textures[ii], ii);

	Renderer::submit(key.encode(), dc);
}

void Renderer3D::draw_cube(const glm::mat4& model_matrix, glm::vec3 color)
{
	VertexArrayHandle VAO = CommonGeometry::get_mesh("cube_lines"_h).VAO;

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
	dc.add_dependency(Renderer::update_uniform_buffer(s_storage.line_ubo, static_cast<void*>(&instance_data), sizeof(LineInstanceData), DataOwnership::Copy));
	Renderer::submit(key.encode(), dc);
}

void Renderer3D::draw_skybox(CubemapHandle cubemap)
{
	if(!cubemap.is_valid())
		return;

	VertexArrayHandle cube = CommonGeometry::get_mesh("cube"_h).VAO;

	RenderState state;
	state.render_target = FramebufferPool::get_framebuffer("LBuffer"_h);
	state.rasterizer_state.cull_mode = CullMode::Front;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_func = DepthFunc::LEqual;
	state.depth_stencil_state.depth_test_enabled = true;
	state.depth_stencil_state.depth_lock = true;
	auto state_flags = state.encode();

	SortKey key;
	key.set_sequence(0, Renderer::next_layer_id(), s_storage.skybox_shader);

	DrawCall dc(DrawCall::Indexed, state_flags, s_storage.skybox_shader, cube);
	dc.set_cubemap(cubemap, 0);
	Renderer::submit(key.encode(), dc);
}


} // namespace erwin