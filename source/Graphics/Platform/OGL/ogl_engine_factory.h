#pragma once

#include "engine_factory.h"

namespace gfx
{

template <>
EngineFactory::EngineComponents EngineFactory::create<DeviceAPI::OpenGL>(const EngineCreateInfo& create_info);

} // namespace gfx