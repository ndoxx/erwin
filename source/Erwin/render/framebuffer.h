#pragma once

#include <cmath>

#include "core/core.h"

namespace erwin
{

class Framebuffer
{
public:
	virtual ~Framebuffer() = default;

	virtual uint32_t get_width() const = 0;
	virtual uint32_t get_height() const = 0;
	virtual bool has_depth() const = 0;

	static WScope<Framebuffer> create(uint32_t width, uint32_t height, bool use_depth_texture);

private:
};


} // namespace erwin