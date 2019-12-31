#pragma once

#include "core/wtypes.h"
#include "render/camera_3d.h"
#include "entity/component_transform.h"
#include "render/handles.h"

namespace erwin
{

struct Material;
// 3D forward renderer front-end
class ForwardRenderer
{
public:
	// Register a shader for forward rendering
	static void register_shader(ShaderHandle shader, UniformBufferHandle material_ubo);
	// Start a new pass
	static void begin_pass(const PerspectiveCamera3D& camera, bool transparent, uint8_t layer_id);
	// End a pass
	static void end_pass();
	// Draw a textured mesh
	// TMP: VertexArrayHandle argument will be replaced by a proper mesh handle
	static void draw_mesh(VertexArrayHandle VAO, const ComponentTransform3D& transform, const Material& material);

	// Stats
	static uint32_t get_draw_call_count();

private:
	friend class Application;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
};

} // namespace erwin