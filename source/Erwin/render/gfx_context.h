#pragma once

namespace erwin
{

class GFXContext
{
public:
	virtual ~GFXContext() {}
	virtual void init() = 0;
	virtual void swap_buffers() = 0;
};

} // namespace erwin