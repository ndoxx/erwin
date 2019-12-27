#pragma once

#include "core/wtypes.h"
#include "render/camera_3d.h"
#include "entity/component_transform.h"
#include "asset/handles.h"

namespace erwin
{

// 3D forward renderer front-end
class ForwardRenderer
{
public:
	// Start a new pass
	static void begin_pass(const PerspectiveCamera3D& camera, bool transparent, uint8_t layer_id);
	// End a pass
	static void end_pass();
	// Draw a colored cube
	static void draw_colored_cube(const ComponentTransform3D& transform, const glm::vec4& tint=glm::vec4(1.f));
	// Draw a textured cube
	static void draw_cube(const ComponentTransform3D& transform, MaterialHandle material, const glm::vec4& tint=glm::vec4(1.f));

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