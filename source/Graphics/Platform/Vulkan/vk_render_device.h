#pragma once
#include "../../render_device.h"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace gfx
{

struct EngineCreateInfo;
class VKRenderDevice : public RenderDevice
{
public:
    VKRenderDevice() = default;
    ~VKRenderDevice();

    inline const auto& get_instance() const { return *instance_; }
    inline const auto& get_physical_device() const { return physical_device_; }
    inline const auto& get_logical_device() const { return *device_; }
    inline const auto& get_graphics_queue() const { return graphics_queue_; }
    inline const auto& get_present_queue() const { return present_queue_; }
    inline auto get_graphics_queue_index() const { return graphics_queue_index_; }
    inline auto get_present_queue_index() const { return present_queue_index_; }

    // Get the best sample count available (in the context of multisampling)
    vk::SampleCountFlagBits get_sample_count(uint32_t max_sample_count) const;
    // Get the depth-stencil format
    vk::Format get_depth_format() const;

    // Create an image view from an image, its format, flags and mipmap levels
    vk::ImageView create_image_view(const vk::Image& image, const vk::Format& format,
                                    const vk::ImageAspectFlags& aspect_flags, uint32_t mip_levels) const;

    friend class EngineFactory;

private:
	// Create a Vulkan instance
    void create_instance(const EngineCreateInfo& create_info);
    // Create a debug messenger wired up to the Kibble logger
    void setup_debug_messenger();
    // Pick the best physical device available
    void select_physical_device(const vk::SurfaceKHR& surface);
    // Create a logical device
    void create_logical_device(const vk::SurfaceKHR& surface);

private:
    vk::UniqueInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    uint32_t graphics_queue_index_;
    uint32_t present_queue_index_;
    std::vector<const char*> validation_layers_;
    std::vector<const char*> device_extensions_;
};

} // namespace gfx