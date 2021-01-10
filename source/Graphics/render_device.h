#pragma once

#include <memory>

namespace gfx
{

enum class DeviceAPI
{
	OpenGL,
	Vulkan
};

class Window;
class RenderDevice
{
public:
	virtual ~RenderDevice() = default;

	static std::unique_ptr<RenderDevice> create(DeviceAPI api, const Window& window);
};

}