#pragma once

#include "render/handles.h"
#include "asset/handles.h"
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
	// Register a material for use with this system
	static void register_material(MaterialHandle material);

	// Start a new deferred rendering pass
	static void begin_deferred_pass();
	// End a deferred rendering pass
	static void end_deferred_pass();
	// Start a new forward rendering pass
	static void begin_forward_pass();
	// End a forward rendering pass
	static void end_forward_pass();
	// Start a line-mode forward pass
	static void begin_line_pass(bool enable_depth_test = true);
	// End forward line-pass
	static void end_line_pass();

	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, MaterialHandle material, void* material_data=nullptr);
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