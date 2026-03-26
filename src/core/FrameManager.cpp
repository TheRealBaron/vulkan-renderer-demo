#include "core/FrameManager.hpp"


FrameManager::FrameManager(GraphicsContext *const gc_ptr, CommandManager *const cmd_mg, uint32_t max_frames_in_flight)
        : gc_ptr(gc_ptr), cmd_mg(cmd_mg), max_frames_in_flight(max_frames_in_flight), current_index(0) {

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VkDevice device = gc_ptr->get_device();
    
    frames.resize(max_frames_in_flight);

    for (FrameData& frame : frames) {
        frame.cmd_buf = cmd_mg->allocate_graphics();
        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &frame.image_available) != VK_SUCCESS) {
            throw std::runtime_error("could not create semaphore");
        }
        if (vkCreateFence(device, &fence_info, nullptr, &frame.in_flight) != VK_SUCCESS) {
            throw std::runtime_error("could not create fence");
        }
    }
}

FrameManager::~FrameManager() {
    VkDevice device = gc_ptr->get_device();
    for (FrameData& frame : frames) {
        vkDestroyFence(device, frame.in_flight, nullptr);
        vkDestroySemaphore(device, frame.image_available, nullptr);
        cmd_mg->destroy_graphics(frame.cmd_buf);
    }
}

void FrameManager::wait_fence() {
    vkWaitForFences(gc_ptr->get_device(), 1, &frames[current_index].in_flight, VK_TRUE, UINT64_MAX);
}

void FrameManager::reset_fence() {
    vkResetFences(gc_ptr->get_device(), 1, &frames[current_index].in_flight);
}

void FrameManager::increment_cur_image() {
    current_index = (current_index + 1) % max_frames_in_flight;
}

