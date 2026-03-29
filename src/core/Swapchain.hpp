#pragma once
#include "core/GraphicsContext.hpp"
#include "resources/DepthBuffer.hpp"


class Swapchain {
public:
    explicit Swapchain(GraphicsContext *const ptr, GLFWwindow *const window);
    ~Swapchain();

    inline uint32_t get_images_cnt() { return static_cast<uint32_t>(images.size()); }
    inline VkSwapchainKHR get_swapchain() { return swapchain; }
    inline VkExtent2D get_extent() { return extent; }
    inline VkRenderPass get_render_pass() { return render_pass; }
    inline VkImageView get_image_view(uint32_t i) { return image_views[i]; }
    inline VkFramebuffer get_framebuffer(uint32_t i) { return framebuffers[i]; }
    inline VkSemaphore get_semaphore(uint32_t i) { return render_finished[i]; }

    uint32_t acquire_next_image(VkSemaphore sem);
    // TODO: add swapchain recreation 
    
private:
    GraphicsContext *const gc_ptr;

    VkSwapchainKHR swapchain;
    VkFormat color_format;
    VkFormat depth_format;
    VkExtent2D extent;
    VkRenderPass render_pass;
    DepthBuffer depth_buffer;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> render_finished;

    void setup_extent_and_format(GLFWwindow* window);
    void create_swapchain(GLFWwindow *const window);
    void extract_images();
    void create_image_views();
    void create_render_pass();
    void create_framebuffers();
    void create_semaphores();
};

