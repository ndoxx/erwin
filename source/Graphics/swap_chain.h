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
};

}