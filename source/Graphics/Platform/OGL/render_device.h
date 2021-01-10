#pragma once
#include "../../render_device.h"

namespace gfx
{

class OGLRenderDevice : public RenderDevice
{
public:
	OGLRenderDevice(const Window& window);
};

}