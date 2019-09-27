#pragma once

#include "render/framebuffer.h"

namespace erwin
{

class OGLFramebuffer: public Framebuffer
{
public:
	OGLFramebuffer(uint32_t width, uint32_t height, bool use_depth_texture);
	~OGLFramebuffer();

	virtual uint32_t get_width() const override  { return width_; }
	virtual uint32_t get_height() const override { return height_; }
	virtual bool has_depth() const override      { return has_depth_; }

private:
	uint32_t width_;
	uint32_t height_;
	bool has_depth_;
	uint32_t rd_handle_;
	uint32_t render_buffer_handle_;
};


} // namespace erwin