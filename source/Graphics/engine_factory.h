#pragma once

#include "device_context.h"
#include "render_device.h"
#include "swap_chain.h"
#include "window.h"
#include <memory>
#include <tuple>

namespace gfx
{

struct EngineCreateInfo
{
    WindowProps window_props;
};

class EngineFactory
{
public:
    using EngineComponents = std::tuple<std::unique_ptr<Window>, std::unique_ptr<RenderDevice>,
                                        std::unique_ptr<SwapChain>, std::unique_ptr<DeviceContext>>;
    template <DeviceAPI API> static EngineComponents create(const EngineCreateInfo& create_info);
    static EngineComponents create(DeviceAPI api, const EngineCreateInfo& create_info);
};

} // namespace gfx