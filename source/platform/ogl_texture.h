#pragma once

#include "render/texture.h"

namespace erwin
{

class OGLTexture2D: public Texture2D
{
public:
	OGLTexture2D(const fs::path filepath);
	OGLTexture2D(const Texture2DDescriptor& descriptor);
	~OGLTexture2D();

	virtual uint32_t get_width() const override;
	virtual uint32_t get_height() const override;

	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

	virtual void* get_native_handle() override;

	inline uint32_t get_handle() const { return rd_handle_; }

private:
	bool handle_filter(uint8_t filter);
	void handle_address_UV(TextureWrap wrap);
	void generate_mipmaps(uint32_t base_level = 0, uint32_t max_level = 3);

private:
	uint32_t width_;
	uint32_t height_;
	uint32_t rd_handle_;
	fs::path filepath_; // TMP: For reload, before we have a proper asset management system
};


} // namespace erwin