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
    auto extensions = vk::enumerateInstanceExtensionProperties();

    KLOG("render", 0) << "Enumerated " << extensions.size() << " extensions: " << std::endl;
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
    auto available_layers = vk::enumerateInstanceLayerProperties();

    KLOG("render", 0) << "Enumerated " << available_layers.size() << " instance layers:" << std::endl;
    for(const auto& props : available_layers)
    {
        KLOGI << props.layerName << std::endl;
    }

    for(const char* layer_name : required)
    {
        KLOG("render", 1) << "Require: " << layer_name << " -> ";

        bool layer_found = false;
        for(const auto& layer_properties : available_layers)
        {
            if(strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if(layer_found)
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

vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info()
{
    return vk::DebugUtilsMessengerCreateInfoEXT(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debug_callback, nullptr);
}

int rate_device_suitability(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface,
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

    auto device_features = device.getFeatures();

    // Application can't function without geometry shaders
    if(!device_features.geometryShader)
        return 0;

    int score = 0;
    auto device_properties = device.getProperties();

    // Discrete GPUs have a significant performance advantage
    if(device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;
    // Maximum possible size of textures affects graphics quality
    score += device_properties.limits.maxImageDimension2D;

    return score;
}

bool check_device_extensions_support(const vk::PhysicalDevice& device, const std::vector<const char*>& extensions)
{
    std::set<std::string> required(extensions.begin(), extensions.end());

    for(const auto& extension : device.enumerateDeviceExtensionProperties())
        required.erase(extension.extensionName);

    return required.empty();
}

QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    QueueFamilyIndices indices;

    auto queue_families = device.getQueueFamilyProperties();

    uint32_t ii = 0;
    for(const auto& queue_family : queue_families)
    {
        if(queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics_family = ii;

        if(queue_family.queueCount > 0 && device.getSurfaceSupportKHR(ii, surface))
            indices.present_family = ii;

        if(indices.is_complete())
            break;

        ++ii;
    }

    return indices;
}

SwapChainSupportDetails query_swap_chain_support(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    SwapChainSupportDetails details;
    // Query basic surface capabilities
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    // Query supported surface formats
    details.formats = device.getSurfaceFormatsKHR(surface);
    // Query supported presentation modes
    details.present_modes = device.getSurfacePresentModesKHR(surface);

    return details;
}

vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats)
{
    if(available_formats.size() == 1 && available_formats[0].format == vk::Format::eUndefined)
        return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    for(const auto& available_format : available_formats)
    {
        if(available_format.format == vk::Format::eB8G8R8A8Unorm &&
           available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return available_format;
    }

    return available_formats[0];
}

vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes)
{
    vk::PresentModeKHR best_mode = vk::PresentModeKHR::eFifo;

    for(const auto& available_present_mode : available_present_modes)
    {
        // Select triple buffering if available
        if(available_present_mode == vk::PresentModeKHR::eMailbox)
            return available_present_mode;
        else if(available_present_mode == vk::PresentModeKHR::eImmediate)
            best_mode = available_present_mode;
    }

    return best_mode;
}

vk::Extent2D choose_swap_extent(uint32_t width, uint32_t height, const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else
    {
        vk::Extent2D actual_extent = {width, height};

        actual_extent.width = std::max(capabilities.minImageExtent.width,
                                       std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height,
                                        std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return actual_extent;
    }
}

} // namespace gfx