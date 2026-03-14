#include <fstream>
#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <optional>
#include <memory>

#define GLFW_INCLUDE_VULKAN

#include "UtilObjects.hpp"
#include "GraphicsContext.hpp"
#include "Swapchain.hpp"
#include "Shader.hpp"

#include "myapp.h"
#include "logger.hpp"
#include "vertex_data.h"


// common objects
GLFWwindow* window;
std::unique_ptr<GraphicsContext> graphics_context;
std::unique_ptr<Swapchain> swapchain;


VkPipelineLayout pipeline_layout;
VkPipeline graphics_pipeline;
VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;
constexpr size_t MAX_BRATCHES_IN_FLIGHT = 3;


//sync objects
std::array<VkSemaphore, MAX_BRATCHES_IN_FLIGHT> image_available_semaphores;
std::vector<VkSemaphore> render_finished_semaphores;
std::array<VkFence, MAX_BRATCHES_IN_FLIGHT> frame_fences; //personal fence for each image
std::vector<VkFence> images_in_flight; // the last fence used by this image in flight
uint32_t current_index;


//data storing objects

//unmutable data
VkBuffer vertex_buffer;
VkDeviceMemory vertex_buffer_memory;
VkBuffer index_buffer;
VkDeviceMemory index_buffer_memory;
//mutable data


// function headers
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
    
    create_graphics_pipeline();

    create_command_pool();

    create_vertex_buffer();
    
    create_index_buffer();
    
    create_command_buffer();

    create_sync_objects();

    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
    current_index = 0;
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


    for (size_t i = 0; i < MAX_BRATCHES_IN_FLIGHT; ++i) {
        vkDestroyFence(mydevice, frame_fences[i], nullptr);
        vkDestroySemaphore(mydevice, image_available_semaphores[i], nullptr);
    }
    for (size_t i = 0; i < render_finished_semaphores.size(); ++i) {
        vkDestroySemaphore(mydevice, render_finished_semaphores[i], nullptr);
    }

    vkDestroyCommandPool(mydevice, command_pool, nullptr);

    vkDestroyPipeline(mydevice, graphics_pipeline, nullptr);
    
    vkDestroyPipelineLayout(mydevice, pipeline_layout, nullptr);
    
    glfwDestroyWindow(window);
    glfwTerminate();
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
    VkDevice mydevice = graphics_context->get_device();
    
    VkShaderModuleCreateInfo module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = src.size() * sizeof(uint32_t),
        .pCode = src.data()
    };
    VkShaderModule res;
    if (vkCreateShaderModule(mydevice, &module_info, nullptr, &res) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }
    return res;
}

void create_graphics_pipeline() {
    VkDevice mydevice = graphics_context->get_device();
    VkExtent2D swap_chain_extent = swapchain->get_extent();

    auto bind_desc = Vertex::get_binding_description();
    auto attr_desc = Vertex::get_attribute_descriptions();

    Shader vertex_shader(graphics_context.get(), "shaders/vertex.spv", ShaderType::VERTEX);
    Shader fragment_shader(graphics_context.get(), "shaders/fragment.spv", ShaderType::FRAGMENT);

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_infos = {
        vertex_shader.get_stage_info(),
        fragment_shader.get_stage_info()
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

    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };


    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 0,
        .pViewports = nullptr, //&viewport,
        .scissorCount = 0,
        .pScissors = nullptr //&scissor
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


    if (vkCreatePipelineLayout(mydevice, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    logger::log(LStatus::INFO, "created pipeline layout");


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
        .renderPass = swapchain->get_render_pass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(
        mydevice, 
        VK_NULL_HANDLE, 
        1, 
        &pipeline_info, 
        nullptr, 
        &graphics_pipeline) != VK_SUCCESS) {

        throw std::runtime_error("could not create graphics pipeline");
    }
    logger::log(LStatus::INFO, "created graphics pipeline");

}


void create_command_pool() {
    VkDevice mydevice = graphics_context->get_device();
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_context->get_graphics_family_index()
    };


    if (vkCreateCommandPool(mydevice, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create command pool");
    }

}


void create_command_buffer() {

    VkDevice mydevice = graphics_context->get_device();
    
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(swapchain->get_images_cnt())
    };

    command_buffers.resize(swapchain->get_images_cnt());
    if (vkAllocateCommandBuffers(mydevice, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate command buffers");
    }

    for (size_t i = 0; i < command_buffers.size(); ++i) {
        vkResetCommandBuffer(command_buffers[i], 0);
        record_command_buffer(command_buffers[i], i);
    }

    logger::log(LStatus::INFO, "allocated command buffers");
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
    
    vkCmdBindPipeline(command_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    
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


void create_sync_objects() {
    VkDevice mydevice = graphics_context->get_device();
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    size_t cnt = swapchain->get_images_cnt();

    images_in_flight.assign(cnt, VK_NULL_HANDLE);
    render_finished_semaphores.resize(cnt);

    for (size_t i = 0; i < cnt; ++i) {
        vkCreateSemaphore(mydevice, &semaphore_info, nullptr, &render_finished_semaphores[i]);
    }
    for (size_t i = 0; i < MAX_BRATCHES_IN_FLIGHT; ++i) {
        vkCreateSemaphore(mydevice, &semaphore_info, nullptr, &image_available_semaphores[i]);
        vkCreateFence(mydevice, &fence_info, nullptr, &frame_fences[i]);
    }
    
    logger::log(LStatus::INFO, "created sync objects");
}


void draw_frame() {
    VkDevice mydevice = graphics_context->get_device();
    VkQueue my_graphics_queue = graphics_context->get_graphics_queue();
    VkQueue my_present_queue = graphics_context->get_present_queue();
    uint32_t image_index;
    
    vkWaitForFences(mydevice, 1, &frame_fences[current_index], VK_TRUE, UINT64_MAX);

    vkAcquireNextImageKHR(
        mydevice,
        swapchain->get_swapchain(),
        UINT64_MAX,
        image_available_semaphores[current_index],
        VK_NULL_HANDLE,
        &image_index
    );

    if (images_in_flight[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(mydevice, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }


    vkResetFences(mydevice, 1, &frame_fences[current_index]);
    images_in_flight[image_index] = frame_fences[current_index];
   
    
    //vkResetCommandBuffer(command_buffers[image_index], 0);
    //record_command_buffer(command_buffers[image_index], image_index);
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


    if (vkQueueSubmit(my_graphics_queue, 1, &submit_info, frame_fences[current_index]) != VK_SUCCESS) {
        throw std::runtime_error("could not submit draw command buffer");
    }

    VkSwapchainKHR myswapchains[] = { swapchain->get_swapchain() };

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphores[image_index],
        .swapchainCount = 1,
        .pSwapchains = myswapchains,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    
    VkResult res = vkQueuePresentKHR(my_present_queue, &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error("could not send image to present queue");
    }
    
    current_index = (current_index + 1) % MAX_BRATCHES_IN_FLIGHT;
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
