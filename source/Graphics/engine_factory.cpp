#include "engine_factory.h"
#include "Platform/OGL/ogl_engine_factory.h"
#include "Platform/Vulkan/vk_engine_factory.h"
#include <kibble/logger/logger.h>

using namespace kb;

namespace gfx
{

std::ostream& operator<<(std::ostream& stream, const Version& ver)
{
    stream << uint32_t(ver.major) << '.' << uint32_t(ver.minor) << '.' << uint32_t(ver.build);
    return stream;
}

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

void EngineFactory::log_create_info(const EngineCreateInfo& create_info)
{
    KLOGN("core") << "Instantiating renderer for application:" << std::endl;
    KLOGI << "Application: " << KS_NAME_ << create_info.application_descriptor.name << KS_ATTR_ << " ("
          << create_info.application_descriptor.version << ')' << std::endl;
    KLOGI << "Engine:      " << KS_NAME_ << create_info.engine_descriptor.name << KS_ATTR_ << " ("
          << create_info.engine_descriptor.version << ')' << std::endl;
}

} // namespace gfx