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
	has_depth_attachment_(depth),
	has_stencil_attachment_(stencil),
	has_cubemap_attachment_(false)
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
	inline bool has_depth() const      { return has_depth_attachment_; }
	inline bool has_stencil() const    { return has_stencil_attachment_; }
	inline bool has_cubemap() const    { return has_cubemap_attachment_; }
	inline const FramebufferLayout& get_layout() const { return layout_; }

	static WScope<Framebuffer> create(uint32_t width, uint32_t height, const FramebufferLayout& layout, bool depth, bool stencil=false);

protected:
	FramebufferLayout layout_;
	uint32_t width_;
	uint32_t height_;
	bool has_depth_attachment_;
	bool has_stencil_attachment_;
	bool has_cubemap_attachment_;
};


} // namespace erwin