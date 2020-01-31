#pragma once

#include "render/camera_3d.h"
#include "render/handles.h"
#include "entity/component_transform.h"
#include "entity/light.h"

namespace erwin
{

struct Material;
class DeferredRenderer
{
public:
	// Register a shader for deferred rendering
	static void register_shader(ShaderHandle shader, UniformBufferHandle material_ubo = UniformBufferHandle());
	static void begin_pass();
	static void end_pass();

	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const glm::mat4& model_matrix, const Material& material);

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};


} // namespace erwin