#pragma once

#include "core/core.h"
#include "render/camera_2d.h"
#include "asset/handles.h"
#include "render/handles.h"
#include "entity/component_transform.h"

namespace erwin
{

// 2D renderer front-end
class Renderer2D
{
public:
	// Start a new pass
	static void begin_pass(const OrthographicCamera2D& camera, bool transparent);
	// End a pass
	static void end_pass();
	// Draw a textured quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_quad(const ComponentTransform2D& transform, TextureAtlasHandle atlas, hash_t tile, const glm::vec4& tint=glm::vec4(1.f));
	// Draw a colored quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_colored_quad(const ComponentTransform2D& transform, const glm::vec4& tint);
	// Render text
	static void draw_text(const std::string& text, FontAtlasHandle font, float x, float y, float scale, const glm::vec4& tint);
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