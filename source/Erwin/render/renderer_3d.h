#pragma once

#include "render/render_state.h"
#include "render/handles.h"
#include "glm/glm.hpp"

/*
 * REFACTOR:
 * 		Organize code into multiple RenderPass derived objects and get rid of this class entirely
 * 		[X] Remove Material's reference to shader and UBO, render passes know about this state
 * 		[ ] Global data like frame UBO data and environment become shared state between render passes
 * 		[ ] Fix the view ID (temporarily) and get rid of Renderer::next_layer_id()
 * 			[ ] View ID and sequence number fields will be filled automatically on pass creation
 */

namespace erwin
{

struct TextureGroup;
struct Mesh;
struct ComponentCamera3D;
struct Transform3D;
struct ComponentDirectionalLight;
struct Environment;

// 3D renderer front-end, handles forward and deferred rendering
class Renderer3D
{
public:
	// Setup frame data
	static void update_camera(const ComponentCamera3D& camera, const Transform3D& transform);
	static void update_light(const ComponentDirectionalLight& dir_light);
	static void update_frame_data();
	// Set an irradiance cubemap for PBR
	static void set_environment(const Environment& environment);
	// Enable/Disable IBL
	static void enable_IBL(bool value);
	// Set IBL ambient strength
	static void set_IBL_ambient_strength(float value);
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
	// TMP
	static void draw_mesh_PBR_opaque(const Mesh& mesh, const glm::mat4& model_matrix, const TextureGroup& texture_group, const void* material_data=nullptr);
	static void draw_quad_billboard_forward(const glm::mat4& model_matrix, const void* material_data=nullptr);
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