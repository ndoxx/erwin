#pragma once

#include <filesystem>
#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace erwin
{

class OGLTexture
{
public:
	OGLTexture() = default;
	virtual ~OGLTexture() = default;

	inline uint32_t get_width() const 	   { return width_; }
	inline uint32_t get_height() const 	   { return height_; }
	inline uint32_t get_mips() const 	   { return mips_; }
	inline uint32_t get_handle() const 	   { return rd_handle_; }
	inline void* get_native_handle() const { return reinterpret_cast<void*>(uint64_t(rd_handle_)); }

	virtual void bind(uint32_t slot = 0) const = 0;
	virtual void unbind() const = 0;

	virtual void generate_mipmaps() const = 0;
	virtual std::pair<uint8_t*, size_t> read_pixels() const { return {nullptr,0}; }

protected:
	uint32_t width_;
	uint32_t height_;
	uint32_t mips_;
	uint32_t rd_handle_;
};

class OGLTexture2D: public OGLTexture
{
public:
	[[deprecated]] OGLTexture2D(const fs::path filepath);
	OGLTexture2D(const Texture2DDescriptor& descriptor);
	virtual ~OGLTexture2D();

	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

	virtual void generate_mipmaps() const override;
	virtual std::pair<uint8_t*, size_t> read_pixels() const override;
};


class OGLCubemap: public OGLTexture
{
public:
	OGLCubemap(const CubemapDescriptor& descriptor);
	virtual ~OGLCubemap();

	virtual void generate_mipmaps() const override;
	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;
};

} // namespace erwin