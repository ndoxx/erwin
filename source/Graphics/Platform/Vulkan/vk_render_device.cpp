// clang-format off
#include "../../window.h"
#include "Platform/Vulkan/vk_render_device.h"
#include "Platform/Vulkan/vk_utils.h"
#include <kibble/logger/logger.h>
#include <map>
#include <set>
#include <string>
// clang-format on

using namespace kb;

namespace gfx
{

#ifdef GFX_VULKAN_ENABLE_VALIDATION_LAYERS
static constexpr bool k_vulkan_enable_validation_layers = true;
#else
static constexpr bool k_vulkan_enable_validation_layers = false;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if(func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pCallback);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
                                   const VkAllocationCallbacks* p_allocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if(func != nullptr)
        func(instance, debug_messenger, p_allocator);
}

VKRenderDevice::~VKRenderDevice()
{
    KLOGN("render") << "[VK] Destroying render device." << std::endl;

    if constexpr(k_vulkan_enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(*instance_, debug_messenger_, nullptr);
}

void VKRenderDevice::create_instance(const std::string& app_name)
{
    KLOGN("render") << "[VK] Creating instance." << std::endl;

    // * Application info
    auto app_info = vk::ApplicationInfo(app_name.c_str(), VK_MAKE_VERSION(1, 0, 0), "Erwin Engine",
                                        VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

    // * Handle extensions
    auto extensions = get_required_extensions();
    if(!check_required_extensions(extensions))
        throw std::runtime_error("Missing required extension!");

    // * Instance creation
    auto create_info = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &app_info, 0, nullptr, // enabled layers
                                              uint32_t(extensions.size()),                      // enabled extensions
                                              extensions.data());

    if constexpr(k_vulkan_enable_validation_layers)
    {
        validation_layers_ = {"VK_LAYER_KHRONOS_validation"};
        if(!check_validation_layer_support(validation_layers_))
            throw std::runtime_error("validation layers requested, but not available!");

        create_info.enabledLayerCount = uint32_t(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }

    try
    {
        instance_ = vk::createInstanceUnique(create_info, nullptr);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("Failed to create instance!");
    }
}

void VKRenderDevice::setup_debug_messenger()
{
    auto debug_create_info = make_debug_messenger_create_info();

    // NOTE: Vulkan-hpp has methods for this, but they trigger linking errors...
    // instance->createDebugUtilsMessengerEXT(debug_create_info);
    // instance->createDebugUtilsMessengerEXTUnique(debug_create_info);
    if(CreateDebugUtilsMessengerEXT(*instance_,
                                    reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info), nullptr,
                                    &debug_messenger_) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug callback!");
}

void VKRenderDevice::select_physical_device(const vk::SurfaceKHR& surface)
{
    KLOGN("render") << "[VK] Picking physical device." << std::endl;

    // Enumerate all available physical devices
    auto devices = instance_->enumeratePhysicalDevices();
    if(devices.size() == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    // Rate devices by suitability and select the best one
    device_extensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::multimap<int, VkPhysicalDevice> candidates;
    for(const auto& device : devices)
    {
        int score = rate_device_suitability(device, surface, device_extensions_);
        candidates.insert(std::make_pair(score, device));
    }
    if(candidates.rbegin()->first > 0)
        physical_device_ = candidates.rbegin()->second;
    else
        throw std::runtime_error("Failed to find a suitable GPU!");

    auto device_properties = physical_device_.getProperties();

    KLOG("render", 1) << "[VK] Selected " << KS_NAME_ << device_properties.deviceName << KC_ << " as a physical device."
                      << std::endl;
}

void VKRenderDevice::create_logical_device(const vk::SurfaceKHR& surface)
{
    KLOGN("render") << "[VK] Creating logical device." << std::endl;
    QueueFamilyIndices indices = find_queue_families(physical_device_, surface);

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

    // Specify queues to be created
    float queue_priority = 1.0f;
    for(uint32_t queue_family : unique_queue_families)
    {
        queue_create_infos.push_back({vk::DeviceQueueCreateFlags(), queue_family,
                                      1, // queueCount
                                      &queue_priority});
    }

    // Specify features to use
    auto device_features = vk::PhysicalDeviceFeatures();

    // Create logical device
    auto create_info =
        vk::DeviceCreateInfo(vk::DeviceCreateFlags(), uint32_t(queue_create_infos.size()), queue_create_infos.data());
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = uint32_t(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

    // No device specific layers in modern Vulkan, this is for compatibility with older implementations
    if constexpr(k_vulkan_enable_validation_layers)
    {
        create_info.enabledLayerCount = uint32_t(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }

    try
    {
        device_ = physical_device_.createDeviceUnique(create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    // Retrieve queue handles
    graphics_queue_ = device_->getQueue(indices.graphics_family.value(), 0);
    present_queue_ = device_->getQueue(indices.present_family.value(), 0);
}

vk::ImageView VKRenderDevice::create_image_view(const vk::Image& image, const vk::Format& format,
                                                const vk::ImageAspectFlags& aspect_flags, uint32_t mip_levels) const
{
    KLOG("render", 0) << "[VK] Creating image view: " << std::endl;
    KLOGI << "Mips: " << KS_VALU_ << mip_levels << std::endl;

    vk::ImageSubresourceRange subresource_range_info{
        aspect_flags, // Flags
        0,            // Base mip level
        mip_levels,   // Mip level count
        0,            // Base array layer
        1             // Layer count
    };

    vk::ImageViewCreateInfo create_info{
        vk::ImageViewCreateFlags(), // Flags
        image,                      // Image
        vk::ImageViewType::e2D,     // View type
        format,                     // Format
        vk::ComponentMapping(),     // Components (swizzle identity by default)
        subresource_range_info      // Subresource range
    };

    return device_->createImageView(create_info);
}

} // namespace gfx