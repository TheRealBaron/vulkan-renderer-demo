#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include <chrono>

#include "utils/UtilObjects.hpp"
#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"
#include "core/PipelineManager.hpp"
#include "core/FrameManager.hpp"

#include "myapp.h"
#include "utils/logger.hpp"
#include "resources/vertex_data.h"
#include "resources/ReadonlyBuffer.hpp"
#include "resources/UniformBuffer.hpp"
#include "animations/animation.hpp"


// common objects
GLFWwindow* window;
std::unique_ptr<GraphicsContext> graphics_context;
std::unique_ptr<Swapchain> swapchain;
std::unique_ptr<PipelineManager> pipeline_manager;
std::unique_ptr<CommandManager> command_manager;
std::unique_ptr<FrameManager> frame_manager;
std::unique_ptr<ReadonlyBuffer> vertex_buffer;
std::unique_ptr<ReadonlyBuffer> index_buffer;
std::unique_ptr<CameraBuffer> camera_buffer;
std::unique_ptr<TransformBuffer> transform_buffer;


std::chrono::time_point<std::chrono::high_resolution_clock> start;
VkDescriptorSetLayout descriptor_set_layout;
VkDescriptorPool descriptor_pool;
std::vector<VkDescriptorSet> descriptor_sets;


void create_descriptor_set_layout(VkDevice device);
void create_descriptor_pool(VkDevice device);
void create_descriptor_sets(size_t cnt, VkDevice device);
void update(float t, uint32_t index);
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


    
    command_manager = std::make_unique<CommandManager>(graphics_context.get());
    frame_manager = std::make_unique<FrameManager>(
        graphics_context.get(), 
        command_manager.get(), 
        swapchain->get_images_cnt()
    ); 

    vertex_buffer = std::make_unique<ReadonlyBuffer>(graphics_context.get(), command_manager.get());
    index_buffer = std::make_unique<ReadonlyBuffer>(graphics_context.get(), command_manager.get());
    camera_buffer = std::make_unique<CameraBuffer>(graphics_context.get(), swapchain->get_images_cnt());
    transform_buffer = std::make_unique<TransformBuffer>(graphics_context.get(), swapchain->get_images_cnt());
    
    vertex_buffer->load(
        static_cast<void const*>(verticies),
        sizeof(verticies),
        ReadonlyBuffer::Usage::VERTEX
    );
    index_buffer->load(
        static_cast<void const*>(indicies), 
        sizeof(indicies),
        ReadonlyBuffer::Usage::INDEX
    );

    camera_buffer->load();
    transform_buffer->load();
    
    create_descriptor_set_layout(graphics_context->get_device());
    
    pipeline_builder->setup_input();
    pipeline_builder->setup_vertex_shader("shaders/vertex.spv");
    pipeline_builder->setup_fragment_shader("shaders/fragment.spv");

    std::array<VkDescriptorSetLayout, 1> lts = {descriptor_set_layout};
    pipeline_builder->setup_layout(lts);

    pipeline_manager->add_pipeline("trivial", *pipeline_builder.get());
   
    create_descriptor_pool(graphics_context->get_device());
    create_descriptor_sets(swapchain->get_images_cnt(), graphics_context->get_device());

    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
    start = std::chrono::high_resolution_clock::now();
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

    vkDestroyDescriptorPool(mydevice, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(mydevice, descriptor_set_layout, nullptr);

    transform_buffer->unload();
    camera_buffer->unload();
    index_buffer->unload();
    vertex_buffer->unload();

    glfwDestroyWindow(window);
    glfwTerminate();
}


void record_command_buffer(VkCommandBuffer command_buf, uint32_t cur_index, uint32_t im_index) {
    
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    
    VkFramebuffer cur_framebuffer = swapchain->get_framebuffer(im_index);
    VkExtent2D swapchain_extent = swapchain->get_extent();
    VkRenderPass render_pass = swapchain->get_render_pass();
    VkPipelineLayout pipeline_layout = pipeline_manager->get_pipeline_layout("trivial");
    
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
        .clearValueCount = 2,
        .pClearValues = clear_values.data()
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

    VkBuffer vertex_bufs[] = {vertex_buffer->get_h()};
    VkDeviceSize offsets[] = {0};
    
    if (vkBeginCommandBuffer(command_buf, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording command buffer");
    }

    vkCmdBeginRenderPass(command_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_manager->get_pipeline("trivial"));
    
    vkCmdSetViewportWithCount(command_buf, 1, &viewport);

    vkCmdSetScissorWithCount(command_buf, 1, &scissor);

    vkCmdBindVertexBuffers(command_buf, 0, 1, vertex_bufs, offsets);
    
    vkCmdBindIndexBuffer(command_buf, index_buffer->get_h(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(
        command_buf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipeline_layout, 0, 1, &descriptor_sets[cur_index], 0, nullptr
    );
    
    vkCmdDrawIndexed(command_buf, static_cast<uint32_t>(sizeof(indicies) / sizeof(int)), 1, 0, 0, 0);
    
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
    auto cur = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(cur - start).count();
    update(t, frame_manager->get_cur_index());
    record_command_buffer(frame_manager->get_current_cmdbuf(), frame_manager->get_cur_index(), image_index);

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


void create_descriptor_set_layout(VkDevice device) {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        VkDescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT

        },
        VkDescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        }
    };
    VkDescriptorSetLayoutCreateInfo desc_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    if (vkCreateDescriptorSetLayout(device, &desc_layout_ci, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create descriptor set layout");
    }
}


void create_descriptor_pool(VkDevice device) {
    VkDescriptorPoolSize cnt = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 16
    };
    VkDescriptorPoolCreateInfo desc_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 8,
        .poolSizeCount = 1,
        .pPoolSizes = &cnt
    };
    if (vkCreateDescriptorPool(device, &desc_pool_ci, nullptr, &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create descriptor pool");
    }
    logger::log(LStatus::INFO, "created descriptor pool");
}


void create_descriptor_sets(size_t cnt, VkDevice device) {
    descriptor_sets.resize(cnt);
    std::vector<VkDescriptorSetLayout> layouts(cnt, descriptor_set_layout);
    VkDescriptorSetAllocateInfo descriptor_sets_ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = static_cast<uint32_t>(cnt),
        .pSetLayouts = layouts.data()
    };
    if (vkAllocateDescriptorSets(device, &descriptor_sets_ai, descriptor_sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate descriptor sets");
    }
    logger::log(LStatus::INFO, "allocated descriptor sets");

    for (size_t i = 0; i < descriptor_sets.size(); ++i) {
        VkDescriptorBufferInfo camera_info = camera_buffer->get_desc_write(i);
        VkDescriptorBufferInfo transform_info = transform_buffer->get_desc_write(i);

        std::array<VkWriteDescriptorSet, 2> write_info = {
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &camera_info
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &transform_info
            }
        };

        vkUpdateDescriptorSets(
            graphics_context->get_device(),
            static_cast<uint32_t>(write_info.size()),
            write_info.data(),
            0,
            nullptr
        );
    }
    logger::log(LStatus::INFO, "updated descriptor sets");

    float ang = glm::radians(15.f);
    glm::vec3 cam_pos = glm::vec3(0.f, glm::sin(ang), glm::cos(ang)) * 6.f;
    glm::vec3 target_pos = glm::vec3(0.f);
    glm::vec3 world_up(0.f, 1.f, 0.f);

    glm::mat4 proj = glm::perspectiveRH(
        glm::radians(60.f),
        (swapchain->get_extent().width + 0.0f) / swapchain->get_extent().height,
        0.1f,
        25.f
    );
    proj[1][1] *= -1.f;
    for (size_t i = 0; i < cnt; ++i) {
        camera_buffer->update(
            CameraBuffer::Ubo { 
                .view = glm::lookAtRH(cam_pos, target_pos, world_up), 
                .proj = proj
            }, 
            i
        );
    }
}


void update(float t, uint32_t index) {
    t = std::fmod(t, 16.5);
    transform_buffer->update(
        TransformBuffer::Ubo(
            animate::drammaticMovement(t),
            animate::drammaticRotation(t)
        ),
        index
    );

}
