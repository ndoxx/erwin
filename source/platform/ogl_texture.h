#pragma once

#include "render/texture.h"

namespace erwin
{

class OGLTexture2D: public Texture2D
{
public:
	OGLTexture2D(const fs::path filepath);
	~OGLTexture2D();

	virtual uint32_t get_width() const override;
	virtual uint32_t get_height() const override;

	virtual void bind(uint32_t slot = 0) override;
	virtual void unbind() override;

private:
	uint32_t width_;
	uint32_t height_;
	uint32_t rd_handle_;
	fs::path filepath_; // TMP: For reload, before we have a proper asset management system
};


} // namespace erwin