#pragma once

#include <map>
#include <cmath>
#include <functional>
#include "core/core.h"
#include "render/framebuffer_layout.h"
#include "render/handles.h"
#include "math/utils.h"
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

	virtual uint32_t get_width(uint32_t) override   { return width_; }
	virtual uint32_t get_height(uint32_t) override { return height_; }
	virtual bool is_fixed() override { return true; }

private:
	uint32_t width_;
	uint32_t height_;
};

class FbRatioConstraint: public FbConstraint
{
public:
	FbRatioConstraint(float width_mul=1.f, float height_mul=1.f): width_mul_(width_mul), height_mul_(height_mul) { }

	virtual uint32_t get_width(uint32_t viewport_width) override   { return uint32_t(std::roundf(width_mul_*float(viewport_width))); }
	virtual uint32_t get_height(uint32_t viewport_height) override { return uint32_t(std::roundf(height_mul_*float(viewport_height))); }

private:
	float width_mul_;
	float height_mul_;
};

// Follow a ratio constraint, but round dimensions to next power of 2
class FbRatioNP2Constraint: public FbConstraint
{
public:
	FbRatioNP2Constraint(float width_mul=1.f, float height_mul=1.f): width_mul_(width_mul), height_mul_(height_mul) { }

	virtual uint32_t get_width(uint32_t viewport_width) override   { return math::np2(uint32_t(std::roundf(width_mul_*float(viewport_width)))); }
	virtual uint32_t get_height(uint32_t viewport_height) override { return math::np2(uint32_t(std::roundf(height_mul_*float(viewport_height)))); }

private:
	float width_mul_;
	float height_mul_;
};

// Follow a ratio constraint, but round dimensions to previous power of 2
class FbRatioPP2Constraint: public FbConstraint
{
public:
	FbRatioPP2Constraint(float width_mul=1.f, float height_mul=1.f): width_mul_(width_mul), height_mul_(height_mul) { }

	virtual uint32_t get_width(uint32_t viewport_width) override   { return math::pp2(uint32_t(std::roundf(width_mul_*float(viewport_width)))); }
	virtual uint32_t get_height(uint32_t viewport_height) override { return math::pp2(uint32_t(std::roundf(height_mul_*float(viewport_height)))); }

private:
	float width_mul_;
	float height_mul_;
};

class EventBus;
class FramebufferPool
{
public:
	// Get a framebuffer handle by name
	static FramebufferHandle get_framebuffer(hash_t name);
	// Visit all framebuffers
	static void traverse_framebuffers(std::function<void(FramebufferHandle)> visitor);
	// Check if a framebuffer has a depth texture attached
	static bool has_depth(hash_t name);
	// Get framebuffer dimensions
	static uint32_t get_width(hash_t name);
	static uint32_t get_height(hash_t name);
	static glm::vec2 get_size(hash_t name);
	static glm::vec2 get_texel_size(hash_t name);
	static uint32_t get_screen_width();
	static uint32_t get_screen_height();
	static glm::vec2 get_screen_size();
	static glm::vec2 get_screen_texel_size();
	// Create a framebuffer inside the pool, specifying a name, size constraints relative to the viewport,
	// a layout for color buffers, and optional depth / depth-stencil textures
	static FramebufferHandle create_framebuffer(hash_t name, WScope<FbConstraint> constraint, uint8_t flags, const FramebufferLayout& layout);

private:
	friend class Application;

	// Initialize pool with default framebuffer dimensions
	static void init(uint32_t initial_width, uint32_t initial_height, EventBus& event_bus /*TMP*/);
	// Destroy all framebuffers stored in this pool
	static void shutdown();
};

} // namespace erwin