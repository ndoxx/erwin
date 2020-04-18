#pragma once

#include <vector>
#include <map>
#include "render/framebuffer.h"

namespace erwin
{

class OGLFramebuffer: public Framebuffer
{
public:
	OGLFramebuffer(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout);
	~OGLFramebuffer();

	virtual void bind() override;
	virtual void unbind() override;
	virtual WRef<Texture> get_shared_texture(uint32_t index=0) override;
	virtual uint32_t get_texture_count() override;
	virtual void screenshot(const std::string& filepath) override;
	virtual void blit_depth(const Framebuffer& source) override;

private:
	void framebuffer_error_report();

private:
	std::vector<WRef<Texture>> textures_;

	uint32_t rd_handle_ = 0;
	uint32_t render_buffer_handle_ = 0;
};


} // namespace erwin