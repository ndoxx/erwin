#pragma once

#include "core/wtypes.h"
#include "render/render_state.h" // For access to enums
#include "render/camera_2d.h"
#include "render/main_renderer.h"
#include "asset/handles.h"
#include "glm/glm.hpp"

namespace erwin
{

// 2D renderer front-end
class Renderer2D
{
public:
	// Start a new pass
	static void begin_pass(const PassState& state, const OrthographicCamera2D& camera, uint8_t layer_id);
	// End a pass
	static void end_pass();
	// Draw a textured quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_quad(const glm::vec4& position, const glm::vec2& scale, hash_t tile, TextureAtlasHandle atlas, const glm::vec4& tint=glm::vec4(1.f));
	static inline void draw_quad(const glm::vec2& position, const glm::vec2& scale, hash_t tile, TextureAtlasHandle atlas, const glm::vec4& tint=glm::vec4(1.f))
	{
		draw_quad(glm::vec4(position, 0.f, 1.f), scale, tile, atlas, tint);
	}
	// Draw a colored quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_colored_quad(const glm::vec4& position, const glm::vec2& scale, const glm::vec4& tint, const glm::vec4& uvs={0.f,0.f,1.f,1.f});
	// Force current batch to be pushed to render queue
	static void flush();

	// Stats
	static uint32_t get_draw_call_count();

private:
	friend class Application;
	friend class AssetManager;

	// Initialize renderer
	static void init();
	// Destroy renderer
	static void shutdown();
	// Register a texture for batching
	static void create_batch(TextureHandle handle);
};

} // namespace erwin