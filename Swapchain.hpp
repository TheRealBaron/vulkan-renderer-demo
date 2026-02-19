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

    inline uint32_t get_images_cnt() { return static_cast<uint32_t>(images.size()); }
    inline VkSwapchainKHR get_swapchain() { return this->swapchain; }
    inline VkExtent2D get_extent() { return extent; }
    inline VkRenderPass get_render_pass() { return render_pass; }
    inline VkImageView get_image_view(uint32_t i) { return image_views[i]; }
    inline VkFramebuffer get_framebuffer(uint32_t i) { return framebuffers[i]; }
    
private:
    GraphicsContext* gc_ptr;

    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    VkRenderPass render_pass;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> framebuffers;

    void create_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    void create_swapchain(GLFWwindow* window);
    void extract_images();
    void create_image_views();
    void create_render_pass();
    void create_framebuffers();
};

