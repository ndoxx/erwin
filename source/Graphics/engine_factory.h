#pragma once

#include "render_context.h"
#include "render_device.h"
#include "swapchain.h"
#include "window.h"
#include <memory>
#include <tuple>
#include <ostream>

namespace gfx
{

struct Version
{
	uint8_t major;
	uint8_t minor;
	uint8_t build;
	friend std::ostream& operator <<(std::ostream&, const Version&);
};

struct ApplicationDescriptor
{
	std::string name;
	Version version;
};

struct EngineDescriptor
{
	std::string name;
	Version version;
};

struct RendererConfig
{
	uint32_t max_sample_count = 1; // 1 = no multisampling, can be 2, 4, 8, 16, 32, 64
};

struct EngineCreateInfo
{
	ApplicationDescriptor application_descriptor;
	EngineDescriptor engine_descriptor;
    WindowProps window_props;
    RendererConfig renderer_config;
};

class EngineFactory
{
public:
    using EngineComponents = std::tuple<std::unique_ptr<Window>, std::unique_ptr<RenderDevice>,
                                        std::unique_ptr<Swapchain>, std::unique_ptr<RenderContext>>;
    template <DeviceAPI API> static EngineComponents create(const EngineCreateInfo& create_info);
    static EngineComponents create(DeviceAPI api, const EngineCreateInfo& create_info);
    static void log_create_info(const EngineCreateInfo& create_info);
};

} // namespace gfx