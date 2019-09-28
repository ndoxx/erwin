#pragma once

#include <vector>
#include "render/framebuffer.h"

namespace erwin
{

class OGLFramebuffer: public Framebuffer
{
public:
	OGLFramebuffer(uint32_t width, uint32_t height, const FrameBufferLayout& layout, bool depth, bool stencil);
	~OGLFramebuffer();

	virtual void bind() override;
	virtual void unbind() override;
	virtual WRef<Texture2D> get_texture(uint32_t index) override;

private:
	void framebuffer_error_report();

private:
	std::vector<WRef<Texture2D>> textures_;
	std::vector<uint32_t> color_buffers_;
	uint32_t rd_handle_ = 0;
	uint32_t render_buffer_handle_ = 0;
};


} // namespace erwin