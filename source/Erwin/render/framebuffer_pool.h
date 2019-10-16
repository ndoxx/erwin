#pragma once

#include <map>
#include "core/wtypes.h"
#include "render/framebuffer.h"
#include "event/window_events.h"

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
	FramebufferPool(uint32_t initial_width, uint32_t initial_height);
	~FramebufferPool();

	// Create a framebuffer inside the pool, specifying a name, size constraints relative to the viewport,
	// a layout for color buffers, and optional depth / depth-stencil textures
	void create_framebuffer(hash_t name, WScope<FbConstraint> constraint, const FrameBufferLayout& layout, bool depth, bool stencil=false);
	// Get a framebuffer by name
	const Framebuffer& get(hash_t name) const;
	// Bind a framebuffer by name
	void bind(hash_t name) const;
	// Check whether a framebuffer is registered to this name
	bool exists(hash_t name) const;
	// Get a framebuffer target texture by framebuffer name and texture index
	const Texture2D& get_texture(hash_t fbname, uint32_t index);
	// Get a framebuffer target texture by framebuffer name and target texture name
	const Texture2D& get_named_texture(hash_t fbname, hash_t texname);
	// Destroy all framebuffers stored in this pool
	void release();

private:
	bool on_framebuffer_resize_event(const FramebufferResizeEvent& event);

private:
	std::map<hash_t, WScope<Framebuffer>> framebuffers_;
	std::map<hash_t, WScope<FbConstraint>> constraints_;

	uint32_t current_width_;
	uint32_t current_height_;
};


} // namespace erwin