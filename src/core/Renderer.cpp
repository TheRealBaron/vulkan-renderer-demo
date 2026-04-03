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


