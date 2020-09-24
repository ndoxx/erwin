#pragma once

namespace erwin
{

class GFXContext
{
public:
	virtual ~GFXContext() = default;
	virtual void init() = 0;
	virtual void swap_buffers() const = 0;
	virtual void make_current() const = 0;
};

} // namespace erwin