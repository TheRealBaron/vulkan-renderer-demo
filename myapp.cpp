#include <fstream>
#include <cassert>
#include <vector>
#include <array>
#include <limits>
#include <algorithm>
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "myapp.h"
#include "logger.hpp"
#include "vertex_data.h"


struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    
    
    QueueFamilyIndices(VkPhysicalDevice ph_dev, VkSurfaceKHR srf) 
        :graphics_family(std::nullopt), present_family(std::nullopt) {
        
        uint32_t qcnt;
        vkGetPhysicalDeviceQueueFamilyProperties(ph_dev, &qcnt, nullptr);
        std::vector<VkQueueFamilyProperties> q_families(qcnt);
        vkGetPhysicalDeviceQueueFamilyProperties(ph_dev, &qcnt, q_families.data());
       

        uint32_t i = 0;
        for (const VkQueueFamilyProperties& qf : q_families) {

            if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                this->graphics_family = i;
            }
            
            VkBool32 present_support = 0x0;
            vkGetPhysicalDeviceSurfaceSupportKHR(ph_dev, i, srf, &present_support);
            if (present_support) {
                this->present_family = i;
            }
            i++;
        
        }
    }
    
    bool is_complete() {
        return graphics_family.has_value() &&
               present_family.has_value();
    }

};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
    
    SwapChainSupportDetails(VkPhysicalDevice ph_dev, VkSurfaceKHR srf) {

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
};


// common vulkan objects
GLFWwindow* window;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physical_device;
VkPhysicalDeviceProperties2 physical_device_properties;
VkPhysicalDeviceFeatures2 physical_device_features;
VkDevice device;
VkQueue graphics_queue, present_queue;
VkSwapchainKHR swap_chain;
VkFormat swap_chain_image_format;
VkExtent2D swap_chain_extent;
std::vector<VkImage> swap_chain_images;
std::vector<VkImageView> swap_chain_image_views;
VkPipelineLayout pipeline_layout;
VkRenderPass render_pass;
VkPipeline graphics_pipeline;
std::vector<VkFramebuffer> swapchain_framebuffers;
VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;
constexpr size_t MAX_BRATCHES_IN_FLIGHT = 3;


//sync objects
std::array<VkSemaphore, MAX_BRATCHES_IN_FLIGHT> image_available_semaphores;
std::vector<VkSemaphore> render_finished_semaphores;
std::array<VkFence, MAX_BRATCHES_IN_FLIGHT> frame_fences;
std::vector<VkFence> images_in_flight;
uint32_t current_index;


//data storing objects


//unmutable data
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_memory;


//mutable data


// function headers
bool device_suitable(VkPhysicalDevice candidate);
VkExtent2D create_extent(const VkSurfaceCapabilitiesKHR& capabilities);
void create_instance();
void pick_qhysical_device();
void create_logical_device();
void create_surface();

void create_swapchain();

void create_image_views();
void read_bytes(const std::string& filename, std::vector<char>& buffer);
void create_render_pass();
void create_graphics_pipeline();
void create_framebuffers();
void create_command_pool();
void create_command_buffer();


void create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& buffer_memory
);

void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);


void create_vertex_buffer();
void create_index_buffer();
uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);


void record_command_buffer(VkCommandBuffer command_buf, uint32_t im_index);
void create_sync_objects();
void draw_frame();
VkShaderModule create_shader_module();


void myapp::run() {
    myapp::initWindow();
    myapp::initVulkan();
    myapp::mainloop();
    myapp::cleanup();
}


static void myapp::initWindow() {
    if (!glfwInit()) {
        throw std::exception("error: could not initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(1440, 810, "hello vulkan", nullptr, nullptr);
    glfwMakeContextCurrent(window);
}


static void myapp::initVulkan() {
    
    create_instance();
    
    create_surface();
    
    pick_qhysical_device();
    
    create_logical_device();

    create_swapchain();

    create_image_views();

    create_render_pass();

    create_graphics_pipeline();
    
    create_framebuffers();

    create_command_pool();

    create_vertex_buffer();
    
    create_index_buffer();
    
    create_command_buffer();

    create_sync_objects();

    current_index = 0;
}


static void myapp::mainloop() {
    while (!glfwWindowShouldClose(window)) {
        draw_frame();
        glfwPollEvents();
    }
}


static void myapp::cleanup() {
    vkDeviceWaitIdle(device);
    
    vkFreeMemory(device, index_buffer_memory, nullptr);
    vkDestroyBuffer(device, index_buffer, nullptr);

    vkFreeMemory(device, vertex_buffer_memory, nullptr);
    vkDestroyBuffer(device, vertex_buffer, nullptr);


    for (size_t i = 0; i < MAX_BRATCHES_IN_FLIGHT; ++i) {
        vkDestroyFence(device, frame_fences[i], nullptr);
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
    }
    for (size_t i = 0; i < swap_chain_images.size(); ++i) {
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
    }

    vkDestroyCommandPool(device, command_pool, nullptr);

    for (auto& framebuffer : swapchain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyPipeline(device, graphics_pipeline, nullptr);

    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    
    vkDestroyRenderPass(device, render_pass, nullptr);
    

    for (VkImageView view : swap_chain_image_views) {
        vkDestroyImageView(device, view, nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
    
    vkDestroyDevice(device, nullptr);
    
    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    vkDestroyInstance(instance, nullptr);
    
    glfwDestroyWindow(window);
    glfwTerminate();

}


void create_instance() {
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
   
    const std::array<const char*, 2> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_KHRONOS_synchronization2"
    };  

    std::string requested = "Requested extentions: ";
    for (const char* extention_name : extensions) {
        requested += extention_name;
        requested += ", ";
    }
    logger::log(requested);
    requested = "Requested validation layers: ";
    for (const char* vlayer : validationLayers) {
        requested += vlayer;
        requested += ", ";
    }
    logger::log(requested);


    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "my vulkan hello world",
        .apiVersion = VK_API_VERSION_1_4
    };


    // Insatance
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };


    if (vkCreateInstance(&inst_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed create vulkan instance");
    }
    
    logger::log("successfully created instance");
}

void pick_qhysical_device() {

    physical_device = VK_NULL_HANDLE;
    uint32_t device_cnt;
    vkEnumeratePhysicalDevices(instance, &device_cnt, nullptr);
    if (device_cnt == 0) {
        throw std::runtime_error("could not find suitable GPU");
    }

    std::vector<VkPhysicalDevice> candidates(device_cnt);
    vkEnumeratePhysicalDevices(instance, &device_cnt, candidates.data());
    

    for (auto candidate : candidates) {
        if (device_suitable(candidate)) {
            physical_device = candidate;
            
            physical_device_properties = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = nullptr
            };
            physical_device_features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = nullptr
            };

            vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties);
            vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);
            logger::log(std::string{"found_suitable device: "} + physical_device_properties.properties.deviceName);
            return;
        }
    }

    throw std::runtime_error("none of found GPUs supported all requested features");
}


bool device_suitable(VkPhysicalDevice candidate) {
    
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

    std::string log_msg = "checking suitability for ";
    log_msg += dprops.properties.deviceName;
    log_msg += ": ";

    bool is_discrete = dprops.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    log_msg += std::to_string(is_discrete) + '|';

    
    bool has_swapchain_support = false;
    for (const auto& prop : props) {
        if (std::string{prop.extensionName} == "VK_KHR_swapchain") {
            has_swapchain_support = true;
            break;
        }
    }
    log_msg += std::to_string(has_swapchain_support) + '|';

    
    bool swap_chain_adequate = false;
    if (has_swapchain_support) {
        SwapChainSupportDetails swapchain_support(candidate, surface);
        swap_chain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }
    log_msg += std::to_string(swap_chain_adequate);

    logger::log(log_msg);

    return dprops.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           has_swapchain_support &&
           swap_chain_adequate;
}


void create_logical_device() {
    QueueFamilyIndices indices(physical_device, surface); 

    if (!indices.is_complete()) {
        throw std::runtime_error("could not find required queue family for this deviece");
    }

    std::array<float, 2> priority = {0.5f, 0.5f};

    std::array<VkDeviceQueueCreateInfo, 2> qinfo = {
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *indices.graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &priority[0]
        },
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = *indices.present_family,
            .queueCount = 1,
            .pQueuePriorities = &priority[1]
        }
    };

    std::array<const char*, 1> dextentions = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo dinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos = qinfo.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = dextentions.data(),
        .pEnabledFeatures = &physical_device_features.features
    };

    
    VkResult res = vkCreateDevice(physical_device, &dinfo, nullptr, &device);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("could not create logical device");
    }

    vkGetDeviceQueue(device, *indices.graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(device, *indices.present_family, 0, &present_queue);
    
    logger::log("successfully created logical device");
}


void create_surface() {
    VkWin32SurfaceCreateInfoKHR sinfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd = glfwGetWin32Window(window)
    };

    if (vkCreateWin32SurfaceKHR(instance, &sinfo, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface");
    }
    logger::log("created surface successfully");
}


VkExtent2D create_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    VkExtent2D res = {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };

    res.width = std::clamp(
        res.width, 
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    res.height = std::clamp(
        res.height, 
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );
    
    return res;
}


void create_swapchain() {
    SwapChainSupportDetails swapchain_support(physical_device, surface);
    VkExtent2D myextent = create_extent(swapchain_support.capabilities);

    uint32_t image_cnt = swapchain_support.capabilities.minImageCount + 1;
    logger::log(std::string{"images in swapchain: "} + std::to_string(image_cnt));

    if (swapchain_support.capabilities.maxImageCount) {
        image_cnt = std::min(image_cnt, swapchain_support.capabilities.maxImageCount);
    }
    
    QueueFamilyIndices indices(physical_device, surface);
    std::array<uint32_t, 2> qindices = {
        *indices.graphics_family,
        *indices.present_family,
    };


    VkSwapchainCreateInfoKHR swapchain_cinfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_cnt,
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = myextent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount = 2,
        .pQueueFamilyIndices = qindices.data(),
        .preTransform = swapchain_support.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };


    if (vkCreateSwapchainKHR(device, &swapchain_cinfo, nullptr, &swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
   

    logger::log("created swapchain");

    swap_chain_image_format = VK_FORMAT_B8G8R8A8_SRGB;
    swap_chain_extent = myextent;

    vkGetSwapchainImagesKHR(device, swap_chain, &image_cnt, nullptr);
    swap_chain_images.resize(image_cnt);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_cnt, swap_chain_images.data());
    logger::log("got " + std::to_string(image_cnt) + " images from swap chain");
}


void create_image_views() {
    uint32_t image_cnt = static_cast<uint32_t>(swap_chain_images.size());

    swap_chain_image_views.resize(image_cnt);
    for (uint32_t i = 0; i < image_cnt; ++i) {
        VkImageViewCreateInfo imv_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swap_chain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swap_chain_image_format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(device, &imv_info, nullptr, &swap_chain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }

    logger::log("created image views from swap chain");
}


void read_bytes(const std::string& filename, std::vector<uint32_t>& buffer) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader binary");
    }
    
    size_t fsize = static_cast<size_t>(file.tellg());
    
    if (fsize % 4 != 0) {
        throw std::runtime_error("the spir-v binary size is != 0 (mod 4)");
    }

    buffer.resize(fsize / 4);
    
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fsize);
    file.close();
}


VkShaderModule create_shader_module(const std::vector<uint32_t>& src) {
    VkShaderModuleCreateInfo module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = src.size() * sizeof(uint32_t),
        .pCode = src.data()
    };
    VkShaderModule res;
    if (vkCreateShaderModule(device, &module_info, nullptr, &res) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }
    return res;
}


void create_render_pass() {
    
    VkAttachmentDescription color_attachment = {
        .format  = swap_chain_image_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };


    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };


    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };


    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };


    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("could not create render pass");
    }
    logger::log("created render pass");
}


void create_graphics_pipeline() {
    
    std::vector<uint32_t> vert_shader_binary;  
    std::vector<uint32_t> frag_shader_binary;

    read_bytes("shaders/vertex.spv", vert_shader_binary);
    read_bytes("shaders/fragment.spv", frag_shader_binary);

    VkShaderModule vertex_module = create_shader_module(vert_shader_binary);
    VkShaderModule fragment_module = create_shader_module(frag_shader_binary);
    logger::log("created shader modules");


    auto bind_desc = Vertex::get_binding_description();
    auto attr_desc = Vertex::get_attribute_descriptions();


    std::array<VkPipelineShaderStageCreateInfo, 2> shader_infos = {
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_module,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_module,
            .pName = "main",
        }
    };


    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bind_desc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_desc.size()),
        .pVertexAttributeDescriptions = attr_desc.data()
    };


    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };


    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swap_chain_extent.width),
        .height = static_cast<float>(swap_chain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };


    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swap_chain_extent
    };


    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };


    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };


    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
   

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };


    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };


    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                          VK_COLOR_COMPONENT_G_BIT | 
                          VK_COLOR_COMPONENT_B_BIT | 
                          VK_COLOR_COMPONENT_A_BIT,
    };


    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = { 0.f, 0.f, 0.f, 0.f }
    };
    

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
    };


    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };


    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    logger::log("created pipeline layout");


    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shader_infos.size()),
        .pStages = shader_infos.data(),
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(
        device, 
        VK_NULL_HANDLE, 
        1, 
        &pipeline_info, 
        nullptr, 
        &graphics_pipeline) != VK_SUCCESS) {

        throw std::runtime_error("could not create graphics pipeline");

    }
    logger::log("created graphics pipeline");
    
    vkDestroyShaderModule(device, fragment_module, nullptr);
    vkDestroyShaderModule(device, vertex_module, nullptr);
}


void create_framebuffers() {
    swapchain_framebuffers.resize(swap_chain_image_views.size());
    
    for (size_t i = 0; i < swapchain_framebuffers.size(); ++i) {
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &swap_chain_image_views[i],
            .width = swap_chain_extent.width,
            .height = swap_chain_extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(device, &fb_info, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("could not create framebuffer");
        }
    }
    logger::log("created frame buffers");

}


void create_command_pool() {
    
    QueueFamilyIndices indices(physical_device, surface);
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphics_family.value()
    };


    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create command pool");
    }

}


void create_command_buffer() {

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(swap_chain_images.size())
    };

    command_buffers.resize(swap_chain_images.size());
    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate command buffers");
    }

    logger::log("allocated command buffers");
}


void record_command_buffer(VkCommandBuffer command_buf, uint32_t im_index) {
    
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pInheritanceInfo = nullptr,
    };

    VkRenderPassBeginInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = swapchain_framebuffers[im_index],
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = swap_chain_extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };
    
    VkViewport viewport = {
        .x = 0.f,
        .y = 0.f,
        .width = static_cast<float>(swap_chain_extent.width),
        .height = static_cast<float>(swap_chain_extent.height),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };
    
    VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = swap_chain_extent
    };

    VkBuffer vertex_bufs[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    
    if (vkBeginCommandBuffer(command_buf, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording command buffer");
    }

    vkCmdBeginRenderPass(command_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    
    vkCmdSetViewport(command_buf, 0, 1, &viewport);

    vkCmdSetScissor(command_buf, 0, 1, &scissor);

    vkCmdBindVertexBuffers(command_buf, 0, 1, vertex_bufs, offsets);
    
    vkCmdBindIndexBuffer(command_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(command_buf, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    
    vkCmdEndRenderPass(command_buf);

    if (vkEndCommandBuffer(command_buf) != VK_SUCCESS) {
        throw std::runtime_error("could not record command buffer");
    }

}


void create_sync_objects() {
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    size_t cnt = swap_chain_images.size();

    images_in_flight.assign(cnt, VK_NULL_HANDLE);
    render_finished_semaphores.resize(cnt);

    for (size_t i = 0; i < cnt; ++i) {
        vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]);
    }
    for (size_t i = 0; i < MAX_BRATCHES_IN_FLIGHT; ++i) {
        vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]);
        vkCreateFence(device, &fence_info, nullptr, &frame_fences[i]);
    }
    
    logger::log("created sync objects");
}


void draw_frame() {

    uint32_t image_index;
    std::string log_msg = "sent to graphics queue&present queue. current_image: ";
    log_msg += std::to_string(current_index);
    
    vkWaitForFences(device, 1, &frame_fences[current_index], VK_TRUE, UINT64_MAX);

    vkAcquireNextImageKHR(
        device,
        swap_chain,
        UINT64_MAX,
        image_available_semaphores[current_index],
        VK_NULL_HANDLE,
        &image_index
    );

    if (images_in_flight[image_index] != VK_NULL_HANDLE) {
        //logger::log("warning: waiting for image " + std::to_string(image_index) + " to free");
        vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }


    vkResetFences(device, 1, &frame_fences[current_index]);
    images_in_flight[image_index] = frame_fences[current_index];
   
    log_msg += ", image_index: " + std::to_string(image_index);
    
    vkResetCommandBuffer(command_buffers[image_index], 0);
    record_command_buffer(command_buffers[image_index], image_index);
    std::array<VkPipelineStageFlags, 1> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; 


    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphores[current_index],
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphores[image_index]
    }; 


    if (vkQueueSubmit(graphics_queue, 1, &submit_info, frame_fences[current_index]) != VK_SUCCESS) {
        throw std::runtime_error("could not submit draw command buffer");
    }


    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphores[image_index],
        .swapchainCount = 1,
        .pSwapchains = &swap_chain,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    
    VkResult res = vkQueuePresentKHR(present_queue, &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("could not send image to present queue");
    }
    
    current_index = (current_index + 1) % MAX_BRATCHES_IN_FLIGHT;
}


uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        VkMemoryPropertyFlags propflags = mem_props.memoryTypes[i].propertyFlags;
        if ((type_filter & (1u << i)) && (propflags & properties) == properties) {
            return i;
        }
    }
   

    throw std::runtime_error("could not find suitable memory type");
}

void create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& buffer_memory) {

    
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
   
    if (vkCreateBuffer(device, &buf_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create vertex buffer");
    }

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device, buffer, &reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = reqs.size,
        .memoryTypeIndex = find_memory_type(reqs.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate gpu memory");
    }

    vkBindBufferMemory(device, buffer, buffer_memory, 0);
}


void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer command_buf;
    
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buf
    };
   

    if (vkAllocateCommandBuffers(device, &alloc_info, &command_buf) != VK_SUCCESS) {
        throw std::runtime_error("could not create temporary command buffer for copying");
    }

    if (vkBeginCommandBuffer(command_buf, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording copy commands");
    }

    vkCmdCopyBuffer(command_buf, src, dst, 1, &copy_region);
    vkEndCommandBuffer(command_buf);

    vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buf);
}


void create_vertex_buffer() {
    VkDeviceSize bufsize = sizeof(vertices[0]) * vertices.size();
    VkBuffer tmp_buf;
    VkDeviceMemory tmp_buf_memory;
    
    create_buffer(
        bufsize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        tmp_buf,
        tmp_buf_memory 
    );

    create_buffer(
        bufsize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertex_buffer,
        vertex_buffer_memory
    );


    void* data;
    if (vkMapMemory(device, tmp_buf_memory, 0, bufsize, 0, &data) != VK_SUCCESS) {
        throw std::runtime_error("could not map gpu memory to virtual adress space");
    }
    memcpy(data, vertices.data(), bufsize);
    vkUnmapMemory(device, tmp_buf_memory);

    copy_buffer(tmp_buf, vertex_buffer, bufsize);

    logger::log("loaded vertex data to gpu");
    vkFreeMemory(device, tmp_buf_memory, nullptr);
    vkDestroyBuffer(device, tmp_buf, nullptr);
}


void create_index_buffer() {
    VkDeviceSize bufsize = sizeof(indices[0]) * indices.size();
    VkBuffer tmp_buf;
    VkDeviceMemory tmp_buf_memory;
    
    create_buffer(
        bufsize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        tmp_buf,
        tmp_buf_memory 
    );
    
    create_buffer(
        bufsize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        index_buffer,
        index_buffer_memory 
    );

    void* data;
    if (vkMapMemory(device, tmp_buf_memory, 0, bufsize, 0, &data) != VK_SUCCESS) {
        throw std::runtime_error("could not map gpu memory to virtual adress space");
    }
    memcpy(data, indices.data(), bufsize);
    vkUnmapMemory(device, tmp_buf_memory);

    copy_buffer(tmp_buf, index_buffer, bufsize);
    
    logger::log("loaded index data to gpu");
    vkFreeMemory(device, tmp_buf_memory, nullptr);
    vkDestroyBuffer(device, tmp_buf, nullptr);
}
