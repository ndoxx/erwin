#pragma once

#include "../../render_context.h"
#include "Platform/Vulkan/vk_render_pass.h"
#include <vulkan/vulkan.hpp>

namespace gfx
{

class VKRenderDevice;
class VKSwapchain;
class VKRenderContext : public RenderContext
{
public:
	VKRenderContext(const VKRenderDevice&, const VKSwapchain&);
	~VKRenderContext();

	void init(uint32_t sample_count);

	// Command buffer factory. Create a command buffer and set it up to begin.
	// Caller has the responsibility to submit it when ready.
	vk::UniqueCommandBuffer begin_command_buffer();
	// Submit a command buffer to the graphics queue
	void submit(const vk::CommandBuffer& command_buffer);


private:
	void create_command_pool();

private:
	vk::RenderPass presentation_pass_;
	vk::CommandPool command_pool_;

	const VKRenderDevice& render_device_;
	const VKSwapchain& swapchain_;
};

}