#include "Platform/Vulkan/vk_render_context.h"
#include "Platform/Vulkan/vk_render_device.h"
#include "Platform/Vulkan/vk_swapchain.h"

#include <kibble/logger/logger.h>
#include <vector>

using namespace kb;

namespace gfx
{

VKRenderContext::VKRenderContext(const VKRenderDevice& rd, const VKSwapchain& sc) : render_device_(rd), swapchain_(sc)
{}

VKRenderContext::~VKRenderContext()
{
    render_device_.get_logical_device().destroyCommandPool(command_pool_);
    render_device_.get_logical_device().destroyRenderPass(presentation_pass_);
}

void VKRenderContext::init(uint32_t sample_count)
{
    KLOGN("render") << "Initializing render context." << std::endl;
    presentation_pass_ = VKRenderPass::create_render_pass(render_device_, swapchain_, sample_count);
    create_command_pool();
}

void VKRenderContext::create_command_pool()
{
    KLOG("render", 1) << "Creating command pool." << std::endl;
    vk::CommandPoolCreateInfo create_info{vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                          render_device_.get_graphics_queue_index()};

    command_pool_ = render_device_.get_logical_device().createCommandPool(create_info);
}

vk::UniqueCommandBuffer VKRenderContext::begin_command_buffer()
{
    // Create a single command buffer
    vk::CommandBufferAllocateInfo alloc_info{command_pool_,                    // Command pool
                                             vk::CommandBufferLevel::ePrimary, // Level
                                             1};                               // Command buffer count

    vk::UniqueCommandBuffer command_buffer{
        std::move(render_device_.get_logical_device().allocateCommandBuffersUnique(alloc_info)[0])};

    // Begin command buffer and pass it to caller
    vk::CommandBufferBeginInfo begin_info{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // Flags
        nullptr                                         // Inheritance info
    };

    command_buffer->begin(begin_info);

    return command_buffer;
}

void VKRenderContext::submit(const vk::CommandBuffer& command_buffer)
{
    // End command buffer
    command_buffer.end();

    // Submit it to the graphics queue
    vk::SubmitInfo submit_info{
        0,               // Wait semaphore count
        nullptr,         // Wait semaphores
        nullptr,         // Wait destination stage mask
        1,               // Command buffer count
        &command_buffer, // Command buffers,
        0,               // Signal semaphore count
        nullptr          // Signal semaphores
    };

    auto result = render_device_.get_graphics_queue().submit(1, &submit_info, vk::Fence());
    if(result != vk::Result::eSuccess)
    {
        KLOGE("render") << "Error while submitting command buffer." << std::endl;
    }
    // Process submission, wait till queue is idle (all commands have been processed)
    render_device_.get_graphics_queue().waitIdle();
}

} // namespace gfx