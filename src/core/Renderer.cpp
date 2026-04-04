#include "core/Renderer.hpp"


Renderer::Renderer(GLFWwindow *const window) {

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
    
    command_manager = std::make_unique<CommandManager>(graphics_context.get());
    
    frame_manager = std::make_unique<FrameManager>(
        graphics_context.get(), 
        command_manager.get(), 
        swapchain->get_images_cnt()
    );
     
}


void Renderer::prepare_for_rendering(Scene *const scene) {
    PipelineBuilder builder(graphics_context.get(), swapchain.get());
    
    scene->configure_pipeline_builder(&builder);
    pipeline_manager->add_pipeline("trivial", builder);

    time.setup_start();
}

void Renderer::draw_frame(Scene *const scene) {
    VkQueue graphics_queue = graphics_context->get_graphics_queue();
    VkQueue present_queue = graphics_context->get_present_queue();
    VkCommandBuffer cur_cmdbuf = frame_manager->get_current_cmdbuf();
    
    frame_manager->wait_fence();
    
    uint32_t image_index = swapchain->acquire_next_image(frame_manager->get_image_available_semaphore());
    
    vkResetCommandBuffer(cur_cmdbuf, 0);
    frame_manager->reset_fence();
    
    float t = time.get();
    scene->update(t, frame_manager->get_cur_index());
    
    record_command_buffer(cur_cmdbuf, scene, frame_manager->get_cur_index(), image_index);

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
    if (res == VK_ERROR_SURFACE_LOST_KHR) {
        throw std::runtime_error("the surface is lost");
    }

    frame_manager->increment_cur_image();
}


void Renderer::record_command_buffer(
    VkCommandBuffer command_buf, 
    Scene *const scene, 
    uint32_t cur_index, 
    uint32_t im_index) {
    
    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    
    VkFramebuffer cur_framebuffer = swapchain->get_framebuffer(im_index);
    VkExtent2D swapchain_extent = swapchain->get_extent();
    VkRenderPass render_pass = swapchain->get_render_pass();
    VkPipelineLayout pipeline_layout = pipeline_manager->get_pipeline_layout("trivial");
    VkPipeline pipeline = pipeline_manager->get_pipeline("trivial");
    
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

    if (vkBeginCommandBuffer(command_buf, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("could not start recording command buffer");
    }

    vkCmdBeginRenderPass(command_buf, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(command_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_manager->get_pipeline("trivial"));
    
    vkCmdSetViewportWithCount(command_buf, 1, &viewport);

    vkCmdSetScissorWithCount(command_buf, 1, &scissor);

    scene->record_draw_commands(command_buf, pipeline_layout, cur_index);
    
    vkCmdEndRenderPass(command_buf);

    if (vkEndCommandBuffer(command_buf) != VK_SUCCESS) {
        throw std::runtime_error("could not record command buffer");
    }
}
