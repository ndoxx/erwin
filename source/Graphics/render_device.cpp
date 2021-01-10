#include "render_device.h"
#include "Platform/OGL/render_device.h"
#include "Platform/Vulkan/render_device.h"
#include <kibble/logger/logger.h>

namespace gfx
{
std::unique_ptr<RenderDevice> RenderDevice::create(DeviceAPI api, const Window& window)
{
    switch(api)
    {
    case DeviceAPI::OpenGL:
        return std::make_unique<OGLRenderDevice>(window);
    default: {
        return std::make_unique<VKRenderDevice>(window);
    }
    }
}

} // namespace gfx