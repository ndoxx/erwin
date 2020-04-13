#pragma once

#include "render/handles.h"
#include "render/render_state.h"
#include "asset/handles.h"
#include "asset/material_common.h"
#include "entity/light.h"
#include "glm/glm.hpp"

namespace erwin
{

class PerspectiveCamera3D;
// 3D renderer front-end, handles forward and deferred rendering
class Renderer3D
{
public:
	// Setup frame data
	static void update_frame_data(const PerspectiveCamera3D& camera, const ComponentDirectionalLight& dir_light);
	static void set_environment_cubemap(CubemapHandle cubemap);
	// Register a shader for use with this system
	static void register_shader(ShaderHandle shader, UniformBufferHandle ubo={}, int shader_flags = 0);
	// Same as previous function, but pass the shader and its UBO via an material
	static void register_shader(MaterialHandle material);
	// Check if a vertex layout is compatible with the attribute layout of the shader inside specified material
	static bool is_compatible(VertexBufferLayoutHandle layout, MaterialHandle material);

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
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, ShaderHandle shader, TextureGroupHandle tg={}, UniformBufferHandle ubo={}, void* material_data=nullptr, uint32_t data_size=0);
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, MaterialHandle material, void* material_data=nullptr);
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