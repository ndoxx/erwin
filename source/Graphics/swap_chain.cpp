#include "Platform/OGL/swap_chain.h"
#include "Platform/Vulkan/swap_chain.h"
#include "render_device.h"
#include <kibble/logger/logger.h>

namespace gfx
{

std::unique_ptr<SwapChain> SwapChain::create(DeviceAPI api, const Window& window, const RenderDevice& rd)
{
    switch(api)
    {
    case DeviceAPI::OpenGL:
        return std::make_unique<OGLSwapChain>(window);
    default: {
        return std::make_unique<VKSwapChain>(window, rd);
    }
    }
}

} // namespace gfx