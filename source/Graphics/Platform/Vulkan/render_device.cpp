#include "Platform/Vulkan/render_device.h"
#include "../../window.h"
#include <kibble/logger/logger.h>
#include <map>
#include <set>
#include <string>

using namespace kb;

namespace gfx
{

#ifdef GFX_VULKAN_ENABLE_VALIDATION_LAYERS
static constexpr bool k_vulkan_enable_validation_layers = true;
#else
static constexpr bool k_vulkan_enable_validation_layers = false;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
                                      const VkAllocationCallbacks* p_allocator,
                                      VkDebugUtilsMessengerEXT* p_debug_messenger)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if(func != nullptr)
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
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
    vkDestroyDevice(device_, nullptr);
    // [BUG] Destroying the debug messenger causes a segfault (Invalid read of size 8
    // @terminator_DestroyDebugUtilsMessengerEXT)
    /*
    if constexpr(k_vulkan_enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    */
    vkDestroyInstance(instance_, nullptr);
}

void VKRenderDevice::create_instance(const std::string& app_name)
{
    KLOGN("render") << "[VK] Creating instance." << std::endl;

    // * Application info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "Erwin Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // * Handle extensions
    auto extensions = get_required_extensions();
    if(!check_required_extensions(extensions))
        throw std::runtime_error("Missing required extension!");

    // * Instance creation
    VkInstanceCreateInfo create_info{};
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    // Validation layer for debug purposes
    if constexpr(k_vulkan_enable_validation_layers)
    {
        validation_layers_ = {"VK_LAYER_KHRONOS_validation"};
        if(!check_validation_layer_support(validation_layers_))
            throw std::runtime_error("validation layers requested, but not available!");

        create_info.enabledLayerCount = uint32_t(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();

        // For instance creation / destruction validation
        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = &debug_create_info;
    }
    else
        create_info.enabledLayerCount = 0;

    create_info.enabledExtensionCount = uint32_t(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if(vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void VKRenderDevice::setup_debug_messenger()
{
    if constexpr(k_vulkan_enable_validation_layers)
        return;

    KLOGN("render") << "[VK] Setting up debug messenger." << std::endl;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    populate_debug_messenger_create_info(debug_create_info);

    if(CreateDebugUtilsMessengerEXT(instance_, &debug_create_info, nullptr, &debug_messenger_) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}

void VKRenderDevice::select_physical_device(const VkSurfaceKHR& surface)
{
    KLOGN("render") << "[VK] Picking physical device." << std::endl;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if(device_count == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(device_count);
    device_extensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::multimap<int, VkPhysicalDevice> candidates;

    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    for(const auto& device : devices)
    {
        int score = rate_device_suitability(device, surface, device_extensions_);
        candidates.insert(std::make_pair(score, device));
    }

    if(candidates.rbegin()->first > 0)
        physical_device_ = candidates.rbegin()->second;
    else
        throw std::runtime_error("Failed to find a suitable GPU!");

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties);

    KLOG("render", 1) << "[VK] Selected " << KS_NAME_ << device_properties.deviceName << KC_ << " as a physical device."
                      << std::endl;
}

void VKRenderDevice::create_logical_device(const VkSurfaceKHR& surface)
{
    KLOGN("render") << "[VK] Creating logical device." << std::endl;

    // Specify queues to be created
    float queue_priority = 1.0f;
    QueueFamilyIndices indices = find_queue_families(physical_device_, surface);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};
    for(uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }
    // Specify features to use
    VkPhysicalDeviceFeatures device_features{};

    // Create logical device
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = uint32_t(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = uint32_t(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

    // No device specific layers in modern Vulkan, this is for compatibility with older implementations
    if constexpr(k_vulkan_enable_validation_layers)
    {
        create_info.enabledLayerCount = uint32_t(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }
    else
        create_info.enabledLayerCount = 0;

    if(vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    // Retrieve queue handles
    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
}

} // namespace gfx