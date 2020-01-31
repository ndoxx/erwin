#pragma once

#include "render/camera_3d.h"
#include "render/handles.h"
#include "entity/light.h"

namespace erwin
{

struct Material;
// 3D forward renderer front-end
class ForwardRenderer
{
public:
	// Register a shader for forward rendering
	static void register_shader(ShaderHandle shader, UniformBufferHandle material_ubo = UniformBufferHandle());
	// Start a new pass
	static void begin_pass();
	// End a pass
	static void end_pass();
	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, const Material& material);

	// Draw a debug cube
	static void begin_line_pass(bool enable_depth_test = true);
	static void end_line_pass();
	static void draw_cube(const glm::mat4& model_matrix, glm::vec3 color);

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin