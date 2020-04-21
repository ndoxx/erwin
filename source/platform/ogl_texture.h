#pragma once

#include "render/texture.h"

namespace erwin
{

class OGLTexture2D: public Texture2D
{
public:
	[[deprecated]] OGLTexture2D(const fs::path filepath);
	OGLTexture2D(const Texture2DDescriptor& descriptor);
	virtual ~OGLTexture2D();

	virtual uint32_t get_width() const override;
	virtual uint32_t get_height() const override;
	virtual uint32_t get_mips() const override;

	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

	virtual void* get_native_handle() override;
	inline uint32_t get_handle() const { return rd_handle_; }

private:
	uint32_t width_;
	uint32_t height_;
	uint32_t mips_;
	uint32_t rd_handle_;
};


class OGLCubemap: public Cubemap
{
public:
	OGLCubemap(const CubemapDescriptor& descriptor);
	virtual ~OGLCubemap();

	virtual uint32_t get_width() const override;
	virtual uint32_t get_height() const override;
	virtual uint32_t get_mips() const override;

	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

	virtual void* get_native_handle() override;
	inline uint32_t get_handle() const { return rd_handle_; }

private:
	uint32_t width_;
	uint32_t height_;
	uint32_t mips_;
	uint32_t rd_handle_;
};

} // namespace erwin