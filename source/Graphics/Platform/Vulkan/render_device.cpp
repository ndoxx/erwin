#include "Platform/Vulkan/render_device.h"
#include "../../window.h"
#include "GLFW/glfw3.h"
#include <cstring>
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                     void* /*p_user_data*/)
{
    uint8_t dlog_severity = 0;
    klog::MsgType dlog_msg_type = klog::MsgType::NORMAL;
    switch(severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        dlog_severity = 0;
        dlog_msg_type = klog::MsgType::NORMAL;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        dlog_severity = 1;
        dlog_msg_type = klog::MsgType::NORMAL;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        dlog_severity = 2;
        dlog_msg_type = klog::MsgType::WARNING;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        dlog_severity = 3;
        dlog_msg_type = klog::MsgType::ERROR;
        break;
    default:
        dlog_severity = 0;
        dlog_msg_type = klog::MsgType::NORMAL;
        break;
    }
    char flavor = 0;
    switch(message_type)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        flavor = 'G';
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        flavor = 'V';
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        flavor = 'P';
        break;
    default:
        flavor = 0;
        break;
    }

    klog::get_log("render"_h, dlog_msg_type, dlog_severity)
        << "[" << flavor << "] " << p_callback_data->pMessage << std::endl;

    return VK_FALSE;
}

VKRenderDevice::VKRenderDevice(const Window& window)
{
    create_instance();
    setup_debug_messenger();
    create_surface(window);
    select_physical_device();
    create_logical_device();
}

VKRenderDevice::~VKRenderDevice()
{
    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if constexpr(k_vulkan_enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

void VKRenderDevice::create_instance()
{
    KLOGN("render") << "Creating instance." << std::endl;

    // * Application info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // * Handle extensions
    /*uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);*/
    auto extensions = get_required_extensions();
    if(!check_required_extensions(extensions))
        throw std::runtime_error("Missing required extension!");

    // * Instance creation
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    // Validation layer for debug purposes
    validation_layers_ = {"VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if constexpr(k_vulkan_enable_validation_layers)
    {
        if(!check_validation_layer_support(validation_layers_))
            throw std::runtime_error("validation layers requested, but not available!");

        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();

        // For instance creation / destruction validation
        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info);
    }
    else
        create_info.enabledLayerCount = 0;

    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    if(vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void VKRenderDevice::setup_debug_messenger()
{
    if constexpr(!k_vulkan_enable_validation_layers)
        return;

    KLOGN("render") << "Setting up debug messenger." << std::endl;

    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    populate_debug_messenger_create_info(create_info);

    if(CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}

void VKRenderDevice::create_surface(const Window& window)
{
    auto* pw = static_cast<GLFWwindow*>(window.get_native());
    if(glfwCreateWindowSurface(instance_, pw, nullptr, &surface_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VKRenderDevice::select_physical_device()
{
    KLOGN("render") << "Picking physical device." << std::endl;

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
        int score = rate_device_suitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    if(candidates.rbegin()->first > 0)
        physical_device_ = candidates.rbegin()->second;
    else
        throw std::runtime_error("Failed to find a suitable GPU!");

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device_, &device_properties);

    KLOG("render", 1) << "Selected " << KS_NAME_ << device_properties.deviceName << KC_ << " as a physical device."
                      << std::endl;
}

void VKRenderDevice::create_logical_device()
{
    KLOGN("render") << "Creating logical device." << std::endl;

    // Specify queues to be created
    float queue_priority = 1.0f;
    QueueFamilyIndices indices = find_queue_families(physical_device_);

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
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

    // No device specific layers in modern Vulkan, this is for compatibility with older implementations
    if constexpr(k_vulkan_enable_validation_layers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info.ppEnabledLayerNames = validation_layers_.data();
    }
    else
        create_info.enabledLayerCount = 0;

    if(vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    // Retrieve queue handles
    // TODO: MOVE to context
    vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);
}

std::vector<const char*> VKRenderDevice::get_required_extensions()
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if constexpr(k_vulkan_enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool VKRenderDevice::check_required_extensions(const std::vector<const char*>& required)
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    KLOG("render", 0) << "Enumerated " << extension_count << " extensions: " << std::endl;
    for(const auto& extension : extensions)
    {
        KLOGI << extension.extensionName << std::endl;
    }

    bool success = true;
    for(const char* extension_name : required)
    {
        KLOG("render", 1) << "Require: " << extension_name << " -> ";

        // Find in required extensions
        bool found = false;
        for(const auto& extension : extensions)
        {
            if(strcmp(extension_name, extension.extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if(found)
        {
            KLOG("render", 1) << KS_GOOD_ << "found" << std::endl;
        }
        else
        {
            KLOG("render", 1) << KS_BAD_ << "not found" << std::endl;
            success = false;
        }
    }

    return success;
}

bool VKRenderDevice::check_validation_layer_support(const std::vector<const char*>& required)
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available.data());

    KLOG("render", 0) << "Enumerated " << layer_count << " instance layers:" << std::endl;
    for(uint32_t ii = 0; ii < layer_count; ++ii)
    {
        KLOGI << available[ii].layerName << std::endl;
    }

    for(const char* layer_name : required)
    {
        KLOG("render", 1) << "Require: " << layer_name << " -> ";
        bool found = false;
        for(const auto& layer_props : available)
        {
            if(strcmp(layer_name, layer_props.layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if(found)
        {
            KLOG("render", 1) << KS_GOOD_ << "found" << std::endl;
        }
        else
        {
            KLOG("render", 1) << KS_BAD_ << "not found" << std::endl;
            return false;
        }
    }
    return true;
}

void VKRenderDevice::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
}

int VKRenderDevice::rate_device_suitability(VkPhysicalDevice device)
{
    int score = 0;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // Discrete GPUs have a significant performance advantage
    if(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;
    // Maximum possible size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;
    // Application can't function without geometry shaders
    if(!device_features.geometryShader)
        return 0;

    // Check support for device extensions
    if(!check_device_extensions_support(device))
        return 0;

    // Check swap chain compatibility with surface
    auto swap_chain_support_details = query_swap_chain_support(device);
    bool swap_chain_adequate =
        !swap_chain_support_details.formats.empty() && !swap_chain_support_details.present_modes.empty();
    if(!swap_chain_adequate)
        return 0;

    // Check support for required queue families
    auto indices = find_queue_families(device);
    if(!indices.is_complete())
        return 0;

    return score;
}

bool VKRenderDevice::check_device_extensions_support(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required(device_extensions_.begin(), device_extensions_.end());
    for(const auto& extension : available_extensions)
    {
        required.erase(extension.extensionName);
        if(required.empty())
            return true;
    }

    return false;
}

QueueFamilyIndices VKRenderDevice::find_queue_families(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    uint32_t ii = 0;
    for(const auto& queue_family : queue_families)
    {
        if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics_family = ii;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, ii, surface_, &present_support);
        if(present_support)
            indices.present_family = ii;

        if(indices.is_complete())
            break;
        ++ii;
    }

    // Assign index to queue families that could be found
    return indices;
}

SwapChainSupportDetails VKRenderDevice::query_swap_chain_support(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    // Query basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    // Query supported surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if(format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    // Query supported presentation modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    if(present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }

    return details;
}

} // namespace gfx