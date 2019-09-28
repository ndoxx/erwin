#pragma once

#include <cmath>

#include "core/core.h"
#include "render/texture.h"

namespace erwin
{

struct FrameBufferLayoutElement
{
	hash_t target_name;
	ImageFormat image_format = ImageFormat::RGBA8;
	uint8_t filter = MIN_LINEAR | MAG_NEAREST;
	TextureWrap wrap = TextureWrap::REPEAT;
};

struct FrameBufferLayout
{
	FrameBufferLayout(const std::initializer_list<FrameBufferLayoutElement>& elements);

    // Iterators for use in range-based for loops
    inline std::vector<FrameBufferLayoutElement>::iterator begin()             { return elements_.begin(); }
    inline std::vector<FrameBufferLayoutElement>::iterator end()               { return elements_.end(); }
    inline std::vector<FrameBufferLayoutElement>::const_iterator begin() const { return elements_.begin(); }
    inline std::vector<FrameBufferLayoutElement>::const_iterator end() const   { return elements_.end(); }

    inline uint32_t get_count() const { return elements_.size(); }

private:
    std::vector<FrameBufferLayoutElement> elements_;
};

class Framebuffer
{
public:
	Framebuffer(uint32_t width, uint32_t height, const FrameBufferLayout& layout, bool depth, bool stencil=false):
	layout_(layout),
	width_(width),
	height_(height),
	has_depth_(depth),
	has_stencil_(stencil)
	{

	}

	virtual ~Framebuffer() = default;
	virtual void bind() = 0;
	virtual void unbind() = 0;

	inline uint32_t get_width() const  { return width_; }
	inline uint32_t get_height() const { return height_; }
	inline bool has_depth() const      { return has_depth_; }
	inline bool has_stencil() const    { return has_stencil_; }
	inline const FrameBufferLayout& get_layout() const { return layout_; }

	static WScope<Framebuffer> create(uint32_t width, uint32_t height, const FrameBufferLayout& layout, bool depth, bool stencil=false);

protected:
	FrameBufferLayout layout_;
	uint32_t width_;
	uint32_t height_;
	bool has_depth_;
	bool has_stencil_;
};


} // namespace erwin