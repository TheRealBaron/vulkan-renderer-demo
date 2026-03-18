#include <vector>
#include <array>
#include <algorithm>
#include <memory>

#define GLFW_INCLUDE_VULKAN

#include "UtilObjects.hpp"
#include "GraphicsContext.hpp"
#include "Swapchain.hpp"
#include "PipelineManager.hpp"
#include "FrameManager.hpp"

#include "myapp.h"
#include "logger.hpp"
#include "vertex_data.h"


// common objects
GLFWwindow* window;
std::unique_ptr<GraphicsContext> graphics_context;
std::unique_ptr<Swapchain> swapchain;
std::unique_ptr<PipelineManager> pipeline_manager;
std::unique_ptr<CommandManager> command_manager;
std::unique_ptr<FrameManager> frame_manager;

//unmutable data
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_memory;

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


void draw_frame();

void myapp::run() {
    myapp::initWindow();
    myapp::initVulkan();
    myapp::mainloop();
    myapp::cleanup();
}


static void myapp::initWindow() {
    if (!glfwInit()) {
        throw std::exception("could not initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(1440, 810, "lighting model demo", nullptr, nullptr);
}


static void myapp::initVulkan() {

    uint32_t req_cnt;
    const char** req_extentions = glfwGetRequiredInstanceExtensions(&req_cnt);
    std::vector<const char*> instance_reqs(req_extentions, req_extentions + req_cnt);
    std::vector<const char*> validation_layers =
#ifdef NDEBUG
    {}
#else
    {"VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"}
#endif
    ;
    std::vector<const char*> gpu_reqs = {"VK_KHR_swapchain"};

    graphics_context = std::make_unique<GraphicsContext>(validation_layers, instance_reqs, gpu_reqs, window);
    
    swapchain = std::make_unique<Swapchain>(graphics_context.get(), window);
    
    pipeline_manager = std::make_unique<PipelineManager>(graphics_context->get_device());

    std::unique_ptr<PipelineBuilder> pipeline_builder = std::make_unique<PipelineBuilder>(
        graphics_context.get(), 
        swapchain.get()
    );

    pipeline_builder->setup_input();
    pipeline_builder->setup_vertex_shader("shaders/vertex.spv");
    pipeline_builder->setup_fragment_shader("shaders/fragment.spv");
    pipeline_builder->setup_layout();

    pipeline_manager->add_pipeline("trivial", *pipeline_builder.get());
    
    command_manager = std::make_unique<CommandManager>(graphics_context.get());
    frame_manager = std::make_unique<FrameManager>(
        graphics_context.get(), 
        command_manager.get(), 
        swapchain->get_images_cnt()
    );
    
    create_vertex_buffer();
    
    create_index_buffer();

    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
}


static void myapp::mainloop() {
    while (!glfwWindowShouldClose(window)) {
        draw_frame();
        glfwPollEvents();
    }
}


static void myapp::cleanup() {
    VkDevice mydevice = graphics_context->get_device();
    vkDeviceWaitIdle(mydevice);
    
    vkFreeMemory(mydevice, index_buffer_memory, nullptr);
    vkDestroyBuffer(mydevice, index_buffer, nullptr);

    vkFreeMemory(mydevice, vertex_buffer_memory, nullptr);
    vkDestroyBuffer(mydevice, vertex_buffer, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}




void record_command_buffer(VkCommandBuffer command_buf, uint32_t im_index) {
    
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    
    VkFramebuffer cur_framebuffer = swapchain->get_framebuffer(im_index);
    VkExtent2D swapchain_extent = swapchain->get_extent();
    VkRenderPass render_pass = swapchain->get_render_pass();
    
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pInheritanceInfo = nullptr,
    };

    VkRenderPassBeginInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = cur_framebuffer,
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = swapchain_extent
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };
    
    VkViewport viewport = {
        .x = 0.f,
        .y = 0.f,
        .width = static_cast<float>(swapchain_extent.width),
        .height = static_cast<float>(swapchain_extent.height),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };
    
    VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = swapchain_extent
    };

    VkBuffer vertex_bufs[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    
    if (vkBeginCommandBuffer(command_buf, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording command buffer");
    }

    vkCmdBeginRenderPass(command_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_manager->get_pipeline("trivial"));
    
    vkCmdSetViewportWithCount(command_buf, 1, &viewport);

    vkCmdSetScissorWithCount(command_buf, 1, &scissor);

    vkCmdBindVertexBuffers(command_buf, 0, 1, vertex_bufs, offsets);
    
    vkCmdBindIndexBuffer(command_buf, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(command_buf, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    
    vkCmdEndRenderPass(command_buf);

    if (vkEndCommandBuffer(command_buf) != VK_SUCCESS) {
        throw std::runtime_error("could not record command buffer");
    }
}


void draw_frame() {
    VkQueue graphics_queue = graphics_context->get_graphics_queue();
    VkQueue present_queue = graphics_context->get_present_queue();
    
    frame_manager->wait_fence();
    
    uint32_t image_index = swapchain->acquire_next_image(frame_manager->get_image_available_semaphore());
    
    frame_manager->reset_fence();
    
    vkResetCommandBuffer(frame_manager->get_current_cmdbuf(), 0);
    record_command_buffer(frame_manager->get_current_cmdbuf(), image_index);

    std::array<VkPipelineStageFlags, 1> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}; 
    std::array<VkSemaphore, 1> wait_semaphores = { frame_manager->get_image_available_semaphore() }; 
    std::array<VkSemaphore, 1> signal_semaphores = { swapchain->get_semaphore(image_index) }; 
    std::array<VkCommandBuffer, 1> cmd_bufs = { frame_manager->get_current_cmdbuf() };
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = cmd_bufs.data(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores.data()
    }; 


    if (vkQueueSubmit(graphics_queue, 1, &submit_info, frame_manager->get_current_fence()) != VK_SUCCESS) {
        throw std::runtime_error("could not submit draw command buffer");
    }

    VkSwapchainKHR myswapchains[] = { swapchain->get_swapchain() };

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = myswapchains,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    
    VkResult res = vkQueuePresentKHR(present_queue, &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("the swapchain is out of date");
    }

    frame_manager->increment_cur_image();
}


uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(graphics_context->get_physical_device(), &mem_props);

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
   
    if (vkCreateBuffer(graphics_context->get_device(), &buf_info, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create vertex buffer");
    }

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(graphics_context->get_device(), buffer, &reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = reqs.size,
        .memoryTypeIndex = find_memory_type(reqs.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(graphics_context->get_device(), &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate gpu memory");
    }

    vkBindBufferMemory(graphics_context->get_device(), buffer, buffer_memory, 0);
}


void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkDevice mydevice = graphics_context->get_device();
    VkCommandPool cmdpool;
    VkCommandBuffer cmdbuffer;

    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_context->get_transfer_family_index()
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
        .pCommandBuffers = &cmdbuffer
    };
   
    if (vkCreateCommandPool(mydevice, &pool_info, nullptr, &cmdpool) != VK_SUCCESS) {
        throw std::runtime_error("could not create command pool");
    } 

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(mydevice, &alloc_info, &cmdbuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create temporary command buffer for copying");
    }

    if (vkBeginCommandBuffer(cmdbuffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording copy commands");
    }

    vkCmdCopyBuffer(cmdbuffer, src, dst, 1, &copy_region);
    vkEndCommandBuffer(cmdbuffer);

    vkQueueSubmit(graphics_context->get_transfer_queue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_context->get_transfer_queue());

    vkFreeCommandBuffers(mydevice, cmdpool, 1, &cmdbuffer);
    vkDestroyCommandPool(mydevice, cmdpool, nullptr);
}


void create_vertex_buffer() {
    VkDevice mydevice = graphics_context->get_device();
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
    if (vkMapMemory(mydevice, tmp_buf_memory, 0, bufsize, 0, &data) != VK_SUCCESS) {
        throw std::runtime_error("could not map gpu memory to virtual adress space");
    }
    memcpy(data, vertices.data(), bufsize);
    vkUnmapMemory(mydevice, tmp_buf_memory);

    copy_buffer(tmp_buf, vertex_buffer, bufsize);

    logger::log(LStatus::INFO, "loaded vertex data to gpu");
    vkFreeMemory(mydevice, tmp_buf_memory, nullptr);
    vkDestroyBuffer(mydevice, tmp_buf, nullptr);
}


void create_index_buffer() {
    VkDevice mydevice = graphics_context->get_device();
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
    if (vkMapMemory(mydevice, tmp_buf_memory, 0, bufsize, 0, &data) != VK_SUCCESS) {
        throw std::runtime_error("could not map gpu memory to virtual adress space");
    }
    memcpy(data, indices.data(), bufsize);
    vkUnmapMemory(mydevice, tmp_buf_memory);

    copy_buffer(tmp_buf, index_buffer, bufsize);
    
    logger::log(LStatus::INFO, "loaded index data to gpu");
    vkFreeMemory(mydevice, tmp_buf_memory, nullptr);
    vkDestroyBuffer(mydevice, tmp_buf, nullptr);
}


