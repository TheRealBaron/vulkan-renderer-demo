#pragma once
#include <vector>

#include "vulkan/vulkan.hpp"


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    
    SwapChainSupportDetails(VkPhysicalDevice ph_dev, VkSurfaceKHR srf);
};


struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    
    QueueFamilyIndices(VkPhysicalDevice ph_dev, VkSurfaceKHR srf);
    bool is_complete();
};
