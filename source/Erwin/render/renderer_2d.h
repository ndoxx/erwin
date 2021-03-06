#pragma once

#include "core/core.h"
#include "render/handles.h"

#include "glm/glm.hpp"

namespace erwin
{

class OrthographicCamera2D;
struct Transform2D;
struct TextureAtlas;
struct FontAtlas;

// 2D renderer front-end
class Renderer2D
{
public:
	// Start a new pass
	static void begin_pass(const OrthographicCamera2D& camera, bool transparent);
	// End a pass
	static void end_pass();
	// Draw a textured quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_quad(const Transform2D& transform, const TextureAtlas& atlas, hash_t tile, const glm::vec4& tint=glm::vec4(1.f));
	// Draw a colored quad. This quad will be batched with others if it passes frustum culling, and instanced on queue flush.
	static void draw_colored_quad(const Transform2D& transform, const glm::vec4& tint);
	// Render text
	static void draw_text(const std::string& text, const FontAtlas& font, float x, float y, float scale, const glm::vec4& tint);
	// Force current batch to be pushed to render queue
	static void flush();

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