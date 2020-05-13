#pragma once

#include <vector>
#include <map>
#include "render/framebuffer_layout.h"
#include "platform/ogl_texture.h"

namespace erwin
{

class OGLFramebuffer
{
public:
	OGLFramebuffer(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout);
	~OGLFramebuffer();

	void bind(uint32_t mip_level=0);
	void unbind();
	WRef<OGLTexture> get_shared_texture(uint32_t index=0);
	uint32_t get_texture_count();
	void screenshot(const std::string& filepath);
	void blit_depth(const OGLFramebuffer& source);

	inline uint32_t get_width() const  { return width_; }
	inline uint32_t get_height() const { return height_; }
	inline uint8_t get_flags() const   { return flags_; }
	inline bool has_depth() const      { return bool(flags_ & FBFlag::FB_DEPTH_ATTACHMENT); }
	inline bool has_stencil() const    { return bool(flags_ & FBFlag::FB_STENCIL_ATTACHMENT); }
	inline bool has_cubemap() const    { return bool(flags_ & FBFlag::FB_CUBEMAP_ATTACHMENT); }
	inline const FramebufferLayout& get_layout() const { return layout_; }

private:
	void framebuffer_error_report();

private:
	FramebufferLayout layout_;
	uint32_t width_;
	uint32_t height_;
	uint8_t flags_;

	std::vector<WRef<OGLTexture>> textures_;

	uint32_t rd_handle_ = 0;
	uint32_t render_buffer_handle_ = 0;
};


} // namespace erwin