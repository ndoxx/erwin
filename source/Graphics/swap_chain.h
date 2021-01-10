#pragma once

#include <memory>
#include "common.h"

namespace gfx
{

class Window;
class RenderDevice;
class SwapChain
{
public:
	virtual ~SwapChain() = default;
	virtual void present() = 0;

	static std::unique_ptr<SwapChain> create(DeviceAPI api, const Window& window, const RenderDevice& rd);
};

}