#pragma once

#include "../Erwin/render/gfx_context.h"

namespace erwin
{

class OGLContext: public GFXContext
{
public:
	OGLContext(void* window_handle);
	virtual ~OGLContext() = default;

	virtual void init() override;
	virtual void swap_buffers() const override;
	virtual void make_current() const override;

private:
	void* window_handle_;
};


} // namespace erwin