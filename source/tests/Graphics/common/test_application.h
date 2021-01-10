#pragma once

#include "graphics.h"

using namespace gfx;

class GfxTestApplication
{
public:
	virtual ~GfxTestApplication() = default;
	virtual bool init() = 0;
	virtual void run() = 0;
	virtual void shutdown() = 0;
};