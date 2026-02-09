#include "UtilObjects.hpp"


SwapChainSupportDetails::SwapChainSupportDetails(VkPhysicalDevice ph_dev, VkSurfaceKHR srf) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ph_dev, srf, &capabilities);
    
    uint32_t format_cnt;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ph_dev, srf, &format_cnt, nullptr);
    formats.assign(format_cnt, VkSurfaceFormatKHR{});
    if (format_cnt) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(ph_dev, srf, &format_cnt, formats.data());
    }

    uint32_t present_mode_cnt;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ph_dev, srf, &present_mode_cnt, nullptr);
    present_modes.assign(present_mode_cnt, VkPresentModeKHR{});
    if (present_mode_cnt) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(ph_dev, srf, &present_mode_cnt, present_modes.data());
    }
}


QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice ph_dev, VkSurfaceKHR srf) 
    :graphics_family(std::nullopt), present_family(std::nullopt) {
        
    uint32_t qcnt;
    vkGetPhysicalDeviceQueueFamilyProperties(ph_dev, &qcnt, nullptr);
    std::vector<VkQueueFamilyProperties> q_families(qcnt);
    vkGetPhysicalDeviceQueueFamilyProperties(ph_dev, &qcnt, q_families.data());
    

    for (uint32_t i = 0; i < q_families.size(); ++i) {
        auto flags = q_families[i].queueFlags;

        if ((flags & VK_QUEUE_GRAPHICS_BIT) && !graphics_family.has_value()) {
            graphics_family = i;
            continue;
        }

        VkBool32 present_support = 0x0;
        vkGetPhysicalDeviceSurfaceSupportKHR(ph_dev, i, srf, &present_support);

        if (present_support && !present_family.has_value()) {
            present_family = i;
            continue;
        }
    }
}


bool QueueFamilyIndices::is_complete() {
    return graphics_family.has_value() &&
           present_family.has_value();
}

