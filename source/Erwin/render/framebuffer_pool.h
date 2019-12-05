#pragma once

#include <map>
#include <cmath>
#include "core/wtypes.h"
#include "render/framebuffer_layout.h"
#include "render/handles.h"
#include "event/window_events.h"
#include "glm/glm.hpp"

namespace erwin
{

// Framebuffer constraint classes
// Used to scale a framebuffer relative to the viewport size, on framebuffer resize events
class FbConstraint
{
public:
	virtual ~FbConstraint() = default;

	virtual uint32_t get_width(uint32_t viewport_width) = 0;
	virtual uint32_t get_height(uint32_t viewport_height) = 0;
	virtual bool is_fixed() { return false; }
};

class FbFixedConstraint: public FbConstraint
{
public:
	FbFixedConstraint(uint32_t width, uint32_t height): width_(width), height_(height) { }

	virtual uint32_t get_width(uint32_t viewport_width) override   { return width_; }
	virtual uint32_t get_height(uint32_t viewport_height) override { return height_; }
	virtual bool is_fixed() override { return true; }

private:
	uint32_t width_;
	uint32_t height_;
};

class FbRatioConstraint: public FbConstraint
{
public:
	FbRatioConstraint(float width_mul=1.f, float height_mul=1.f): width_mul_(width_mul), height_mul_(height_mul) { }

	virtual uint32_t get_width(uint32_t viewport_width) override   { return uint32_t(std::roundf(width_mul_*viewport_width)); }
	virtual uint32_t get_height(uint32_t viewport_height) override { return uint32_t(std::roundf(height_mul_*viewport_height)); }

private:
	float width_mul_;
	float height_mul_;
};

class FramebufferPool
{
public:
	static void init(uint32_t initial_width, uint32_t initial_height);
	// Destroy all framebuffers stored in this pool
	static void shutdown();
	// Get a framebuffer handle by name
	static FramebufferHandle get_framebuffer(hash_t name);
	// Check if a framebuffer has a depth texture attached
	static bool has_depth(hash_t name);
	// Get framebuffer dimensions
	static uint32_t get_width(hash_t name);
	static uint32_t get_height(hash_t name);
	static glm::vec2 get_size(hash_t name);
	static glm::vec2 get_texel_size(hash_t name);
	// Create a framebuffer inside the pool, specifying a name, size constraints relative to the viewport,
	// a layout for color buffers, and optional depth / depth-stencil textures
	static FramebufferHandle create_framebuffer(hash_t name, WScope<FbConstraint> constraint, const FramebufferLayout& layout, bool depth, bool stencil=false);
};

} // namespace erwin