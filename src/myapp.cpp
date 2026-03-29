#include <vector>
#include <array>
#include <algorithm>
#include <memory>


#include "utils/UtilObjects.hpp"
#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"
#include "core/PipelineManager.hpp"
#include "core/FrameManager.hpp"

#include "myapp.h"
#include "utils/logger.hpp"
#include "resources/vertex_data.h"
#include "resources/ReadonlyBuffer.hpp"


// common objects
GLFWwindow* window;
std::unique_ptr<GraphicsContext> graphics_context;
std::unique_ptr<Swapchain> swapchain;
std::unique_ptr<PipelineManager> pipeline_manager;
std::unique_ptr<CommandManager> command_manager;
std::unique_ptr<FrameManager> frame_manager;
std::unique_ptr<ReadonlyBuffer> vertex_buffer;
std::unique_ptr<ReadonlyBuffer> index_buffer;


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
    

    vertex_buffer = std::make_unique<ReadonlyBuffer> (graphics_context.get(), command_manager.get());
    index_buffer = std::make_unique<ReadonlyBuffer> (graphics_context.get(), command_manager.get());
    
    vertex_buffer->load(
        static_cast<void const*>(vertices.data()),
        vertices.size() * sizeof(Vertex),
        ReadonlyBuffer::Usage::VERTEX
    );
    index_buffer->load(
        static_cast<void const*>(indices.data()),
        indices.size() * sizeof(uint32_t),
        ReadonlyBuffer::Usage::INDEX
    );


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
    

    index_buffer->unload();
    vertex_buffer->unload();

    glfwDestroyWindow(window);
    glfwTerminate();
}


void record_command_buffer(VkCommandBuffer command_buf, uint32_t im_index) {
    
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    
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


