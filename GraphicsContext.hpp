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
        GLFWwindow* window
    );

    ~GraphicsContext();
    
    VkInstance get_instance() { return instance; }
    VkPhysicalDevice get_physical_device() { return physical_device; }
    VkDevice get_device() { return device; }
    VkSurfaceKHR get_surface() { return surface; }
    VkQueue get_graphics_queue() { return graphics_queue; }
    VkQueue get_present_queue() { return present_queue; }

private:
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device; 
    VkSurfaceKHR surface;
    VkQueue graphics_queue;
    VkQueue present_queue;
    //TODO: add GPU memory allocation infrastructure

    void create_instance(
        const std::vector<const char*>& validation_layers,
        const std::vector<const char*>& extentions
    );

    VkBool32 physical_device_suitable(VkPhysicalDevice candidate);

    void find_physical_device();

    void create_surface(GLFWwindow* window);
    
    void create_device(const std::vector<const char*>& req_gpu_extentions);
};


