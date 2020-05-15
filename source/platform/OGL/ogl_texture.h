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
	inline ImageFormat get_format() const  { return format_; }
	inline uint8_t get_filter() const      { return filter_; }
	inline TextureWrap get_wrap() const    { return wrap_; }
	inline void* get_native_handle() const { return reinterpret_cast<void*>(uint64_t(rd_handle_)); }

	virtual void bind(uint32_t slot = 0) const = 0;
	virtual void unbind() const = 0;

	virtual void generate_mipmaps() const = 0;
	virtual std::pair<uint8_t*, size_t> read_pixels() const { return {nullptr,0}; }

protected:
	uint32_t width_ = 0;
	uint32_t height_ = 0;
	uint32_t mips_ = 0;
	uint32_t rd_handle_ = 0;

	ImageFormat format_ = ImageFormat::RGBA8;
	uint8_t filter_ = TextureFilter::MIN_NEAREST | TextureFilter::MAG_NEAREST;
	TextureWrap wrap_ = TextureWrap::REPEAT;
};

class OGLTexture2D: public OGLTexture
{
public:
	OGLTexture2D() = default;
	OGLTexture2D(const Texture2DDescriptor& descriptor);
	virtual ~OGLTexture2D();

	void init(const Texture2DDescriptor& descriptor);
	void release();
	inline bool is_initialized() const { return initialized_; }

	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

	virtual void generate_mipmaps() const override;
	virtual std::pair<uint8_t*, size_t> read_pixels() const override;

private:
	bool initialized_ = false;
};


class OGLCubemap: public OGLTexture
{
public:
	OGLCubemap() = default;
	OGLCubemap(const CubemapDescriptor& descriptor);
	virtual ~OGLCubemap();

	void init(const CubemapDescriptor& descriptor);
	void release();
	inline bool is_initialized() const { return initialized_; }

	virtual void generate_mipmaps() const override;
	virtual void bind(uint32_t slot = 0) const override;
	virtual void unbind() const override;

private:
	bool initialized_ = false;
};

} // namespace erwin