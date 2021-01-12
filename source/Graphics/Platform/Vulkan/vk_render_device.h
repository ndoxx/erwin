#pragma once
#include "../../render_device.h"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace gfx
{

class VKRenderDevice : public RenderDevice
{
public:
    VKRenderDevice() = default;
    ~VKRenderDevice();

    inline const vk::Instance& get_instance() const { return *instance_; }
    inline const vk::PhysicalDevice& get_physical_device() const { return physical_device_; }
    inline const vk::Device& get_logical_device() const { return *device_; }

    friend class EngineFactory;

private:
    void create_instance(const std::string& app_name);
    void setup_debug_messenger();
    void select_physical_device(const vk::SurfaceKHR& surface);
    void create_logical_device(const vk::SurfaceKHR& surface);

private:
    vk::UniqueInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    vk::Queue graphics_queue_;
    vk::Queue present_queue_;
    std::vector<const char*> validation_layers_;
    std::vector<const char*> device_extensions_;
};

} // namespace gfx