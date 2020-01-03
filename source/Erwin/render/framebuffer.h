#pragma once

#include <cmath>

#include "core/core.h"
#include "render/framebuffer_layout.h"

namespace erwin
{

class Framebuffer
{
public:
	Framebuffer(uint32_t width, uint32_t height, const FramebufferLayout& layout, bool depth, bool stencil=false):
	layout_(layout),
	width_(width),
	height_(height),
	has_depth_(depth),
	has_stencil_(stencil)
	{

	}

	virtual ~Framebuffer() = default;
	virtual void bind() = 0;
	virtual void unbind() = 0;
	virtual WRef<Texture2D> get_shared_texture(uint32_t index=0) = 0;
	virtual const Texture2D& get_texture(uint32_t index=0) = 0;
	virtual const Texture2D& get_named_texture(hash_t name) = 0;
	virtual uint32_t get_texture_count() = 0;
	virtual void screenshot(const std::string& filepath) = 0;

	inline uint32_t get_width() const  { return width_; }
	inline uint32_t get_height() const { return height_; }
	inline bool has_depth() const      { return has_depth_; }
	inline bool has_stencil() const    { return has_stencil_; }
	inline const FramebufferLayout& get_layout() const { return layout_; }

	static WScope<Framebuffer> create(uint32_t width, uint32_t height, const FramebufferLayout& layout, bool depth, bool stencil=false);

protected:
	FramebufferLayout layout_;
	uint32_t width_;
	uint32_t height_;
	bool has_depth_;
	bool has_stencil_;
};


} // namespace erwin