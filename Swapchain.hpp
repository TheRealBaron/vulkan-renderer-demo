#pragma once
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vulkan/vulkan.hpp"
#include "GraphicsContext.hpp"


class Swapchain {
public:
    explicit Swapchain(GraphicsContext* ptr, GLFWwindow* window);
    ~Swapchain();
    
    inline VkSwapchainKHR get_swapchain() { return swapchain; }
    inline VkSwapchainKHR get_get_extent() { return extent; }
    inline VkRenderPass get_render_pass() { return render_pass; }
    inline VkImageView get_image_view(uint32_t i) { return image_views[i]; }
    inline VkImageView get_framebuffer(uint32_t i) { return framebuffer[i]; }
    
private:
    const GraphicsContext* gc_ptr = nullptr;

    VkSwapchainKHR swapchain;
    Vkformat format;
    Vkextent extent;
    VkRenderPass render_pass;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> framebuffers;

    void create_extent(const VkSurfaceCapabilitiesKHR& capabilities, const GLFWwindow* window);
    void create_swapchain(VkDevice mydevice, VkSurfaceKHR surface, uint32_t image_cnt);
    void create_image_views();
    void create_render_pass();
    void create_framebuffers();
};

