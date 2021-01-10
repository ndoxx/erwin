#pragma once
#include "../../render_device.h"
#include <vulkan/vulkan.h>
#include <vector>

#define GFX_VULKAN_ENABLE_VALIDATION_LAYERS

namespace gfx
{

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;

	inline bool is_complete()
	{
		return graphics_family.has_value() && present_family.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class VKRenderDevice : public RenderDevice
{
public:
	VKRenderDevice(const Window& window);
	~VKRenderDevice();

	inline const VkPhysicalDevice& get_physical_device() const { return physical_device_; }
	inline const VkDevice& get_logical_device() const { return device_; }
	inline const VkSurfaceKHR& get_surface() const { return surface_; }
	inline QueueFamilyIndices find_queue_families() const { return find_queue_families(physical_device_); }
	inline SwapChainSupportDetails query_swap_chain_support() const { return query_swap_chain_support(physical_device_); }

private:
	void create_instance();
	void setup_debug_messenger();
	void create_surface(const Window& window);
	void select_physical_device();
	void create_logical_device();

	std::vector<const char*> get_required_extensions();
	bool check_required_extensions(const std::vector<const char*>& required);
	bool check_validation_layer_support(const std::vector<const char*>& required);
	void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
	int rate_device_suitability(VkPhysicalDevice device);
	bool check_device_extensions_support(VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(VkPhysicalDevice device) const;
	SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) const;

private:
	VkInstance instance_;
    VkDebugUtilsMessengerEXT debug_messenger_;
    VkSurfaceKHR surface_;
	VkPhysicalDevice physical_device_;
	VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;
    std::vector<const char*> validation_layers_;
    std::vector<const char*> device_extensions_;
};

}