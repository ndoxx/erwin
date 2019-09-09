#pragma once

#include "../Erwin/render/gfx_context.h"

namespace erwin
{

class OGLContext: public GFXContext
{
public:
	OGLContext(void* window_handle);
	~OGLContext();

	virtual void init() override;
	virtual void swap_buffers() override;

private:
	void* window_handle_;
};


} // namespace erwin