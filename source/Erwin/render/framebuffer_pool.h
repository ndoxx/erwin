#pragma once

#include <unordered_map>
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
	FbRatioConstraint(float width_mul, float height_mul): width_mul_(width_mul), height_mul_(height_mul) { }

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

	void create_framebuffer(hash_t name, WScope<FbConstraint> constraint, const FrameBufferLayout& layout, bool depth, bool stencil=false);

	const Framebuffer& get_framebuffer(hash_t name) const;

	void bind(hash_t name) const;

	void release();

private:
	bool on_framebuffer_resize_event(const FramebufferResizeEvent& event);

private:
	std::unordered_map<hash_t, WScope<Framebuffer>> framebuffers_;
	std::unordered_map<hash_t, WScope<FbConstraint>> constraints_;

	uint32_t current_width_;
	uint32_t current_height_;
};


} // namespace erwin