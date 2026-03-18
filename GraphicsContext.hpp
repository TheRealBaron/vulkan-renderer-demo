#pragma once
#include <vector>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"


class GraphicsContext {
public:

    GraphicsContext();

    explicit GraphicsContext(
        const std::vector<const char*>& req_validation_layers,
        const std::vector<const char*>& req_instance_extentions,
        const std::vector<const char*>& req_gpu_extentions,
        GLFWwindow *const window
    );

    ~GraphicsContext();
    
    VkInstance get_instance() { return instance; }
    VkPhysicalDevice get_physical_device() { return physical_device; }
    VkDevice get_device() { return device; }
    VkSurfaceKHR get_surface() { return surface; }
    VkQueue get_graphics_queue() { return graphics_queue; }
    VkQueue get_present_queue() { return present_queue; }
    VkQueue get_transfer_queue() { return transfer_queue; }
    uint32_t get_graphics_family_index() { return graphics_family; }
    uint32_t get_present_family_index() { return present_family; }
    uint32_t get_transfer_family_index() { return transfer_family; }

private:
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device; 
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
    uint32_t graphics_family;
    uint32_t present_family;
    uint32_t transfer_family;
    //TODO: add GPU memory allocation infrastructure

    void create_instance(
        const std::vector<const char*>& validation_layers,
        const std::vector<const char*>& extentions
    );

    VkBool32 physical_device_suitable(VkPhysicalDevice candidate);

    void find_physical_device();

    void create_surface(GLFWwindow *const window);
    
    void create_device(const std::vector<const char*>& req_gpu_extentions);
};


