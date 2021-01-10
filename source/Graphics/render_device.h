#pragma once

#include <memory>
#include "common.h"

namespace gfx
{

class Window;
class RenderDevice
{
public:
	virtual ~RenderDevice() = default;

	static std::unique_ptr<RenderDevice> create(DeviceAPI api, const Window& window);
};

}