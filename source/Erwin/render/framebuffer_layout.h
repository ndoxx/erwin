#pragma once

#include <vector>

#include "render/texture_common.h"
#include "core/core.h"

namespace erwin
{

struct FramebufferLayoutElement
{
	hash_t target_name;
	ImageFormat image_format = ImageFormat::RGBA8;
	uint8_t filter = MIN_LINEAR | MAG_NEAREST;
	TextureWrap wrap = TextureWrap::REPEAT;
    uint8_t mips = 0;
    bool lazy_mipmap = false;
};

struct FramebufferLayout
{
    FramebufferLayout() = default;
	explicit FramebufferLayout(const std::initializer_list<FramebufferLayoutElement>& elements): elements_(elements) { }
    FramebufferLayout(FramebufferLayoutElement* elements, uint32_t count);

    // Iterators for use in range-based for loops
    inline std::vector<FramebufferLayoutElement>::iterator begin()             { return elements_.begin(); }
    inline std::vector<FramebufferLayoutElement>::iterator end()               { return elements_.end(); }
    inline std::vector<FramebufferLayoutElement>::const_iterator begin() const { return elements_.begin(); }
    inline std::vector<FramebufferLayoutElement>::const_iterator end() const   { return elements_.end(); }

    inline const FramebufferLayoutElement& operator[](size_t index) const { return elements_[index]; }

    inline size_t get_count() const { return elements_.size(); }
    inline const FramebufferLayoutElement* data() const { return elements_.data(); }

private:
    std::vector<FramebufferLayoutElement> elements_;
};

enum FBFlag: uint8_t
{
    FB_NONE               = 0,
    FB_DEPTH_ATTACHMENT   = (1<<0),
    FB_STENCIL_ATTACHMENT = (1<<1),
    FB_CUBEMAP_ATTACHMENT = (1<<2)
};

} // namespace erwin