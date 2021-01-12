#pragma once

#include <memory>
#include "common.h"

namespace gfx
{

class Swapchain
{
public:
	virtual ~Swapchain() = default;
	virtual void present() = 0;
};

}