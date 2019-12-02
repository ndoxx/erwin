#pragma once

#include "core/wtypes.h"
#include "render/render_state.h" // For access to enums
#include "render/camera_3d.h"
#include "render/main_renderer.h"
#include "glm/glm.hpp"

namespace erwin
{

// 3D forward renderer front-end
class ForwardRenderer
{
public:
	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
	// Start a new pass
	static void begin_pass(const PassState& state, const PerspectiveCamera3D& camera, uint8_t layer_id);
	// End a pass
	static void end_pass();
	// Draw a colored cube.
	static void draw_colored_cube(const glm::vec3& position, float scale, const glm::vec4& tint=glm::vec4(1.f));
	// Force current batch to be pushed to render queue
	static void flush();

	// Stats
	static uint32_t get_draw_call_count();
};

} // namespace erwin