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
};

}