#pragma once

#include <cmath>

#include "core/core.h"
#include "render/framebuffer_layout.h"

namespace erwin
{

class Framebuffer
{
public:
	Framebuffer(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout):
	layout_(layout),
	width_(width),
	height_(height),
	flags_(flags)
	{

	}

	virtual ~Framebuffer() = default;
	virtual void bind() = 0;
	virtual void unbind() = 0;
	virtual WRef<Texture> get_shared_texture(uint32_t index=0) = 0;
	virtual uint32_t get_texture_count() = 0;
	virtual void screenshot(const std::string& filepath) = 0;
	virtual void blit_depth(const Framebuffer& source) = 0;

	inline uint32_t get_width() const  { return width_; }
	inline uint32_t get_height() const { return height_; }
	inline uint8_t get_flags() const   { return flags_; }
	inline bool has_depth() const      { return bool(flags_ & FBFlag::FB_DEPTH_ATTACHMENT); }
	inline bool has_stencil() const    { return bool(flags_ & FBFlag::FB_STENCIL_ATTACHMENT); }
	inline bool has_cubemap() const    { return bool(flags_ & FBFlag::FB_CUBEMAP_ATTACHMENT); }
	inline const FramebufferLayout& get_layout() const { return layout_; }

	static WScope<Framebuffer> create(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout);

protected:
	FramebufferLayout layout_;
	uint32_t width_;
	uint32_t height_;
	uint8_t flags_;
};


} // namespace erwin