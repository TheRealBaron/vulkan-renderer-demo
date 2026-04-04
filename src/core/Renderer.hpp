#pragma once


#include "utils/UtilObjects.hpp"
#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"
#include "core/PipelineManager.hpp"
#include "core/FrameManager.hpp"

#include "myapp.h"
#include "utils/logger.hpp"
#include "utils/Time.hpp"
#include "resources/vertex_data.h"
#include "resources/ReadonlyBuffer.hpp"
#include "scene/Scene.hpp"


class Renderer {
public:
    Renderer(GLFWwindow *const window);

    inline GraphicsContext *const get_context() { return graphics_context.get(); }
    inline Swapchain *const get_swapchain() { return swapchain.get(); }
    inline CommandManager *const get_command_manager() { return command_manager.get(); }
    inline uint32_t get_framebuffer_amount() { return swapchain->get_images_cnt(); }
    
    void prepare_for_rendering(Scene *const scene);
    void draw_frame(Scene *const scene);
    void record_command_buffer(VkCommandBuffer command_buf, 
                               Scene *const scene, 
                               uint32_t cur_index, 
                               uint32_t im_index);

private:
    std::unique_ptr<GraphicsContext> graphics_context;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<PipelineManager> pipeline_manager;
    std::unique_ptr<CommandManager> command_manager;
    std::unique_ptr<FrameManager> frame_manager;
    Time time;
};

