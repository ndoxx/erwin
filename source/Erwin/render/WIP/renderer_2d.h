#pragma once

#include "core/wtypes.h"
#include "render/render_state.h" // For access to enums
#include "render/camera_2d.h"
#include "render/WIP/main_renderer.h"
#include "glm/glm.hpp"

namespace erwin
{
namespace WIP
{

// 2D renderer front-end
class Renderer2D
{
public:
	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();

	// Start a new pass
	static void begin_pass(const PassState& state, const OrthographicCamera2D& camera);
	// End a pass
	static void end_pass();
	// Draw a textured quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_quad(const glm::vec2& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas);
	// Same as before, but specifying a non-zero z coordinate
	// static void draw_quad(const glm::vec3& position, const glm::vec2& scale, const glm::vec4& uvs, hash_t atlas);
};

} // namespace WIP
} // namespace erwin