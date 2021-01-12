#include "engine_factory.h"
#include "Platform/OGL/ogl_engine_factory.h"
#include "Platform/Vulkan/vk_engine_factory.h"

namespace gfx
{

EngineFactory::EngineComponents EngineFactory::create(DeviceAPI api, const EngineCreateInfo& create_info)
{
    switch(api)
    {
    case DeviceAPI::OpenGL:
        return EngineFactory::create<DeviceAPI::OpenGL>(create_info);
    case DeviceAPI::Vulkan:
        return EngineFactory::create<DeviceAPI::Vulkan>(create_info);
    }
}

} // namespace gfx