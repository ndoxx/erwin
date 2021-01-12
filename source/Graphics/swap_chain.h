#pragma once

#include <memory>
#include "common.h"

namespace gfx
{

class SwapChain
{
public:
	virtual ~SwapChain() = default;
	virtual void present() = 0;
};

}