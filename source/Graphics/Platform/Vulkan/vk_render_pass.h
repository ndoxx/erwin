#pragma once

#include <vulkan/vulkan.hpp>

namespace gfx
{

class VKRenderDevice;
class VKSwapchain;

class VKRenderPass
{
public:
	static vk::RenderPass create_render_pass(const VKRenderDevice&, const VKSwapchain&, uint32_t max_sample_count);
};

}