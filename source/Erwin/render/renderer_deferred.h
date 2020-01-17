#pragma once

#include "render/camera_3d.h"
#include "entity/component_transform.h"
#include "render/handles.h"
#include "render/light.h"

namespace erwin
{

struct Material;
class DeferredRenderer
{
public:
	// Register a shader for deferred rendering
	static void register_shader(ShaderHandle shader, UniformBufferHandle material_ubo = UniformBufferHandle());
	static void begin_pass(const PerspectiveCamera3D& camera, const DirectionalLight& dir_light);
	static void end_pass();

	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material);

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};


} // namespace erwin