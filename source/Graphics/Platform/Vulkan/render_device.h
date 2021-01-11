#pragma once
#include "../../render_device.h"
#include "utils.h"
#include <vector>

namespace gfx
{

class VKRenderDevice : public RenderDevice
{
public:
    VKRenderDevice() = default;
    ~VKRenderDevice();

    inline const VkInstance& get_instance() const { return instance_; }
    inline const VkPhysicalDevice& get_physical_device() const { return physical_device_; }
    inline const VkDevice& get_logical_device() const { return device_; }

    friend class EngineFactory;

private:
    void create_instance(const std::string& app_name);
    void setup_debug_messenger();
    void select_physical_device(const VkSurfaceKHR& surface);
    void create_logical_device(const VkSurfaceKHR& surface);

private:
    VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    std::vector<const char*> validation_layers_;
    std::vector<const char*> device_extensions_;
};

} // namespace gfx