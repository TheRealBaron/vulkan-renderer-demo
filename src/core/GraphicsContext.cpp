#define VMA_IMPLEMENTATION
#include "core/GraphicsContext.hpp"
#include "utils/UtilObjects.hpp"
#include "utils/logger.hpp"


GraphicsContext::GraphicsContext() :
        instance        (VK_NULL_HANDLE),
        physical_device (VK_NULL_HANDLE),
        surface         (VK_NULL_HANDLE),
        device          (VK_NULL_HANDLE),
        graphics_queue  (VK_NULL_HANDLE),
        present_queue   (VK_NULL_HANDLE){}



GraphicsContext::GraphicsContext(
    const std::vector<const char*>& req_validation_layers,
    const std::vector<const char*>& req_instance_extentions,
    const std::vector<const char*>& req_gpu_extentions,
    GLFWwindow *const window)
    :
        instance        (VK_NULL_HANDLE),
        physical_device (VK_NULL_HANDLE),
        surface         (VK_NULL_HANDLE),
        device          (VK_NULL_HANDLE),
        graphics_queue  (VK_NULL_HANDLE),
        present_queue   (VK_NULL_HANDLE)
    {

    if (!window) {
        throw std::runtime_error("graphics context requires correctly initialized window object");
    }
    
    create_instance(req_validation_layers, req_instance_extentions);

    create_surface(window);

    find_physical_device();

    create_device(req_gpu_extentions); 

    create_allocator();
}

GraphicsContext::~GraphicsContext() {
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}


void GraphicsContext::create_instance(
    const std::vector<const char*>& validation_layers,
    const std::vector<const char*>& extensions) {

    logger::log(LStatus::INFO, "requested extentions: ", extensions);

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "my vulkan hello world",
        .apiVersion = VK_API_VERSION_1_4
    };

    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };


    if (vkCreateInstance(&inst_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed create vulkan instance");
    }
    
    logger::log(LStatus::INFO, "successfully created instance");
}


void GraphicsContext::create_surface(GLFWwindow* window) {
    if (!window) {
        throw std::runtime_error("cannot create surface: window is invalid");
    }

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("could not create surface");
    }
    logger::log(LStatus::INFO, "Surface created: {}", (void*)surface);
}


VkBool32 GraphicsContext::physical_device_suitable(VkPhysicalDevice candidate) {
    VkPhysicalDeviceProperties2 dprops = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = nullptr
    };
    VkPhysicalDeviceFeatures2 dfeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr
    };
    
    vkGetPhysicalDeviceProperties2(candidate, &dprops);
    vkGetPhysicalDeviceFeatures2(candidate, &dfeatures);

    uint32_t ext_cnt;
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ext_cnt, nullptr);
    std::vector<VkExtensionProperties> props(ext_cnt);
    vkEnumerateDeviceExtensionProperties(candidate, nullptr, &ext_cnt, props.data());

    VkBool32 is_discrete = dprops.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    VkBool32 has_swapchain_support = false;
    for (const auto& prop : props) {
        if (std::string{prop.extensionName} == "VK_KHR_swapchain") {
            has_swapchain_support = true;
            break;
        }
    }

    VkBool32 swap_chain_adequate = false;
    if (has_swapchain_support) {
        SwapChainSupportDetails swapchain_support(candidate, surface);
        swap_chain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    logger::log(
        LStatus::INFO,
        "checking suitability for {}: {}|{}|{}",
        dprops.properties.deviceName, is_discrete, has_swapchain_support, swap_chain_adequate
    );

    return is_discrete &&
           has_swapchain_support &&
           swap_chain_adequate;

}


void GraphicsContext::find_physical_device() {
    physical_device = VK_NULL_HANDLE;
    uint32_t device_cnt;
    vkEnumeratePhysicalDevices(instance, &device_cnt, nullptr);
    if (device_cnt == 0) {
        throw std::runtime_error("could not find suitable GPU");
    }

    std::vector<VkPhysicalDevice> candidates(device_cnt);
    vkEnumeratePhysicalDevices(instance, &device_cnt, candidates.data());
    

    for (auto candidate : candidates) {
        if (physical_device_suitable(candidate)) {
            physical_device = candidate;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physical_device, &props);
            logger::log(LStatus::INFO, "Physical device found: {}", props.deviceName);
            return;
        }
    }

    throw std::runtime_error("none of found GPUs supported all requested features");
}


void GraphicsContext::create_device(const std::vector<const char*>& req_gpu_extentions) {
    QueueFamilyIndices qindices(physical_device, surface); 

    if (!qindices.is_complete()) {
        throw std::runtime_error("could not find all required queue families for this device");
    }
    logger::log(LStatus::INFO, "got queue family indices: {}, {}, {}",
                *qindices.graphics_family,
                *qindices.present_family,
                *qindices.transfer_family);


    VkPhysicalDeviceFeatures2 dfeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    };
    vkGetPhysicalDeviceFeatures2(physical_device, &dfeatures);

    std::array<float, 3> priority = {1.f, 1.f, 1.f};

    std::array<VkDeviceQueueCreateInfo, 3> qinfo = {
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *qindices.graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &priority[0]
        },
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *qindices.present_family,
            .queueCount = 1,
            .pQueuePriorities = &priority[1]
        },
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *qindices.transfer_family,
            .queueCount = 1,
            .pQueuePriorities = &priority[2]
        }
    };


    VkDeviceCreateInfo dinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(qinfo.size()),
        .pQueueCreateInfos = qinfo.data(),
        .enabledExtensionCount = static_cast<uint32_t>(req_gpu_extentions.size()),
        .ppEnabledExtensionNames = req_gpu_extentions.data(),
        .pEnabledFeatures = &dfeatures.features
    };

    
    VkResult res = vkCreateDevice(physical_device, &dinfo, nullptr, &device);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("could not create logical device");
    }

    graphics_family = *qindices.graphics_family;
    present_family = *qindices.present_family;
    transfer_family = *qindices.transfer_family;

    vkGetDeviceQueue(device, *qindices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, *qindices.present_family, 0, &present_queue);
    vkGetDeviceQueue(device, *qindices.transfer_family, 0, &transfer_queue);
    
    logger::log(LStatus::INFO, "successfully created logical device");
}

void GraphicsContext::create_allocator() {

    VmaAllocatorCreateInfo allocator_ci = {
        .physicalDevice = physical_device,
        .device = device,
        .instance = instance
    };
    if (vmaCreateAllocator(&allocator_ci, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("could not create allocator");
    }
    

}

