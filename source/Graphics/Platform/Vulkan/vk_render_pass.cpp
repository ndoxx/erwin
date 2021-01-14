#include "Platform/Vulkan/vk_render_pass.h"
#include "Platform/Vulkan/vk_render_device.h"
#include "Platform/Vulkan/vk_swapchain.h"
#include <kibble/logger/logger.h>

using namespace kb;

namespace gfx
{

vk::RenderPass VKRenderPass::create_render_pass(const VKRenderDevice& render_device, const VKSwapchain& swapchain,
                                                uint32_t max_sample_count)
{
    KLOG("render", 1) << "Creating render pass." << std::endl;

    auto color_format = swapchain.get_color_format();
    auto multi_sampling_level = render_device.get_sample_count(max_sample_count);
    auto depth_format = render_device.get_depth_format();

    KLOGI << "Multisampling: " << KS_VALU_ << 'x' << uint32_t(multi_sampling_level) << std::endl;

    // Declare attachments
    vk::AttachmentDescription color_attachment{
        vk::AttachmentDescriptionFlags(), // Flags
        color_format,                     // Format
        multi_sampling_level,             // Samples
        vk::AttachmentLoadOp::eClear,     // Load operation -> clear color before use
        vk::AttachmentStoreOp::eStore,    // Store operation -> we want to store the output
        vk::AttachmentLoadOp::eDontCare,  // Stencil load operation -> color attachment so no stencil op
        vk::AttachmentStoreOp::eDontCare, // Stencil store operation
        vk::ImageLayout::eUndefined,      // Initial layout
        vk::ImageLayout::ePresentSrcKHR}; // Final layout

    vk::AttachmentDescription depth_testing_attachment{
        vk::AttachmentDescriptionFlags(),                 // Flags
        depth_format,                                     // Format
        multi_sampling_level,                             // Samples -> same sampling level as color attachment
        vk::AttachmentLoadOp::eClear,                     // Load operation -> clear before use
        vk::AttachmentStoreOp::eDontCare,                 // Store operation -> no depth-stencil operation no store
        vk::AttachmentLoadOp::eDontCare,                  // Stencil load operation
        vk::AttachmentStoreOp::eDontCare,                 // Stencil store operation
        vk::ImageLayout::eUndefined,                      // Initial layout
        vk::ImageLayout::eDepthStencilAttachmentOptimal}; // Final layout

    vk::AttachmentDescription multi_sampling_attachment{
        vk::AttachmentDescriptionFlags(), // Flags
        color_format,                     // Format
        vk::SampleCountFlagBits::e1,      // Samples -> the output of this attachment will not be multisampled again
        vk::AttachmentLoadOp::eDontCare,  // Load operation
        vk::AttachmentStoreOp::eStore,    // Store operation -> the output should be stored for later presentation
        vk::AttachmentLoadOp::eDontCare,  // Stencil load operation
        vk::AttachmentStoreOp::eDontCare, // Stencil store operation
        vk::ImageLayout::eUndefined,      // Initial layout
        vk::ImageLayout::ePresentSrcKHR}; // Final layout -> layout is the source of the presentation

    // Each attachment description has a reference to be used in the subpasses
    // clang-format off
    vk::AttachmentReference color_attachment_reference{
        0,                                         // Attachment index
        vk::ImageLayout::eColorAttachmentOptimal}; // Layout
    vk::AttachmentReference depth_testing_attachment_reference{
        1,                                                // Attachment index
        vk::ImageLayout::eDepthStencilAttachmentOptimal}; // Layout

    vk::AttachmentReference multi_sampling_attachment_reference{
    	2,                                         // Attachment index
    	vk::ImageLayout::eColorAttachmentOptimal}; // Layout
    // clang-format on

    // List of all attachments
    std::vector<vk::AttachmentDescription> attachments{color_attachment, depth_testing_attachment};

    // Describe subpass
    vk::SubpassDescription subpass{vk::SubpassDescriptionFlags(),    // Flags
                                   vk::PipelineBindPoint::eGraphics, // Pipeline bind point
                                   0,                                // Input attachment count -> no input attachment
                                   nullptr,                          // Input attachments
                                   1,                                // Color attachments count
                                   &color_attachment_reference,      // Color attachments
                                   nullptr, // Resolve attachments -> this attachment resolves the final image
                                   &depth_testing_attachment_reference, // Depth stencil attachments
                                   0,                                   // Preserve attachments count
                                   nullptr};                            // Preserve attachments

    // Multisampling
    // We can't provide a multisampling attachment as a resolve attachment if the sample count is
    // set to 1, due to Vulkan specifications. So my strategy was to prepare everything as if no multisampling
    // was needed, but apply a correction here in case we do want to multisample.
    if(multi_sampling_level != vk::SampleCountFlagBits::e1)
    {
        attachments[0].setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
        subpass.setPResolveAttachments(&multi_sampling_attachment_reference);
        attachments.push_back(multi_sampling_attachment);
    }

    // We only have one subpass, but we need to declare a dependency anyway
    auto access_read_write = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    vk::SubpassDependency subpass_dependency{
        0,                                                 // Source subpass index -> only one subpass at pos 0
        0,                                                 // Destination subpass index
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // Source access mask -> subpass happens after blending
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // Destination access mask
        vk::AccessFlags(),                                 // Source access flags -> we need both read and write
        access_read_write,                                 // Destination access flags
        vk::DependencyFlagBits::eByRegion}; // Dependency flags -> needed because src / dst subpass idx are the same

    // Create render pass
    vk::RenderPassCreateInfo render_pass_create_info{vk::RenderPassCreateFlags(),               // Flags
                                                     static_cast<uint32_t>(attachments.size()), // Attachment count
                                                     attachments.data(),                        // Attachments
                                                     1,                                         // Subpass count
                                                     &subpass,                                  // Subpasses
                                                     1,                                         // Dependency count
                                                     &subpass_dependency};

    return render_device.get_logical_device().createRenderPass(render_pass_create_info);
}

} // namespace gfx