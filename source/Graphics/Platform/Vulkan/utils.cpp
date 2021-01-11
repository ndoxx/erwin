#include "Platform/Vulkan/utils.h"
#include "GLFW/glfw3.h"
#include <cstring>
#include <kibble/logger/logger.h>
#include <set>

using namespace kb;

namespace gfx
{

#ifdef GFX_VULKAN_ENABLE_VALIDATION_LAYERS
static constexpr bool k_vulkan_enable_validation_layers = true;
#else
static constexpr bool k_vulkan_enable_validation_layers = false;
#endif

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
    char flavor = 'U';
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
        flavor = 'U';
        break;
    }

    klog::get_log("vulkan"_h, dlog_msg_type, dlog_severity)
        << "[" << flavor << "] " << p_callback_data->pMessage << std::endl;

    return VK_FALSE;
}

std::vector<const char*> get_required_extensions()
{
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if constexpr(k_vulkan_enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool check_required_extensions(const std::vector<const char*>& required)
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

bool check_validation_layer_support(const std::vector<const char*>& required)
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

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info)
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

int rate_device_suitability(const VkPhysicalDevice& device, const VkSurfaceKHR& surface,
                            const std::vector<const char*>& extensions)
{
    // Check support for device extensions
    if(!check_device_extensions_support(device, extensions))
        return 0;

    // Check swap chain compatibility with surface
    auto swap_chain_support_details = query_swap_chain_support(device, surface);
    bool swap_chain_adequate =
        !swap_chain_support_details.formats.empty() && !swap_chain_support_details.present_modes.empty();
    if(!swap_chain_adequate)
        return 0;

    // Check support for required queue families
    auto indices = find_queue_families(device, surface);
    if(!indices.is_complete())
        return 0;

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // Application can't function without geometry shaders
    if(!device_features.geometryShader)
        return 0;

    int score = 0;
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    // Discrete GPUs have a significant performance advantage
    if(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;
    // Maximum possible size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;

    return score;
}

bool check_device_extensions_support(const VkPhysicalDevice& device, const std::vector<const char*>& extensions)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required(extensions.begin(), extensions.end());
    for(const auto& extension : available_extensions)
    {
        required.erase(extension.extensionName);
        if(required.empty())
            return true;
    }

    return false;
}

QueueFamilyIndices find_queue_families(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, ii, surface, &present_support);
        if(present_support)
            indices.present_family = ii;

        if(indices.is_complete())
            break;
        ++ii;
    }

    // Assign index to queue families that could be found
    return indices;
}

SwapChainSupportDetails query_swap_chain_support(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    // Query basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Query supported surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if(format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    // Query supported presentation modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
    if(present_mode_count != 0)
    {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for(const auto& format : available_formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    // Select triple buffering if available
    for(const auto& mode : available_present_modes)
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actual_extent = {width, height};
        actual_extent.width = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height,
                                        std::min(capabilities.maxImageExtent.height, actual_extent.height));
        return actual_extent;
    }
}

} // namespace gfx