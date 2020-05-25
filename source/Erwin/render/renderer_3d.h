#pragma once

#include "render/handles.h"
#include "render/render_state.h"
#include "asset/handles.h"
#include "asset/material.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentCamera3D;
struct ComponentTransform3D;
struct ComponentDirectionalLight;

// 3D renderer front-end, handles forward and deferred rendering
class Renderer3D
{
public:
	// Setup frame data
	static void update_camera(const ComponentCamera3D& camera, const ComponentTransform3D& transform);
	static void update_light(const ComponentDirectionalLight& dir_light);
	static void update_frame_data();
	// Set an irradiance cubemap for PBR
	static void set_environment(CubemapHandle irradiance, CubemapHandle prefiltered);
	// Enable/Disable IBL
	static void enable_IBL(bool value);
	// Set IBL ambient strength
	static void set_IBL_ambient_strength(float value);
	// Register a shader for use with this system
	static void register_shader(ShaderHandle shader);
	// Check if a vertex layout is compatible with the attribute layout of the shader inside specified material
	//static bool is_compatible(VertexBufferLayoutHandle layout, const Material& material);

	// Show UV in flat shading
	static void debug_show_uv(bool enabled=true);

	// Generate a cubemap from a 2:1 equirectangular HDR texture
	static CubemapHandle generate_cubemap_hdr(TextureHandle hdr_tex, uint32_t size);
	// Generate a pre-computed environment convolution map for diffuse IBL from an environment cubemap
	static CubemapHandle generate_irradiance_map(CubemapHandle env_map);
	// Generate a pre-filtered environment map for specular IBL from an environment cubemap
	static CubemapHandle generate_prefiltered_map(CubemapHandle env_map, uint32_t source_resolution);

	// Start a new deferred rendering pass
	static void begin_deferred_pass();
	// End a deferred rendering pass
	static void end_deferred_pass();
	// Start a new forward rendering pass
	static void begin_forward_pass(BlendState blend_state = BlendState::Alpha);
	// End a forward rendering pass
	static void end_forward_pass();
	// Start a line-mode forward pass
	static void begin_line_pass(bool enable_depth_test = true);
	// End forward line-pass
	static void end_line_pass();

	// Draw a textured mesh
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, ShaderHandle shader, const TextureGroup& tg={}, UniformBufferHandle ubo={}, void* material_data=nullptr, uint32_t data_size=0);
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, const Material& material, void* material_data=nullptr);
	// Render a cubemap as a skybox (whole pass)
	static void draw_skybox(CubemapHandle cubemap);
	// Draw a debug cube
	static void draw_cube(const glm::mat4& model_matrix, glm::vec3 color);

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin