#pragma once
#include "GraphicsContext.hpp"
#include "CommandManager.hpp"



class FrameManager {
public:
    FrameManager(GraphicsContext *const gc_ptr, CommandManager *const cmd_mg, uint32_t max_frames_in_flight);
    ~FrameManager();
    
    inline VkCommandBuffer get_current_cmdbuf() {
        return frames[current_index].cmd_buf;
    }
    
    inline VkSemaphore get_image_available_semaphore() { 
        return frames[current_index].image_available;
    } 
    
    inline VkFence get_current_fence() {
        return frames[current_index].in_flight;
    };

    void wait_fence();
    void reset_fence();
    void increment_cur_image();

private:
    struct FrameData {
        VkCommandBuffer cmd_buf;
        VkSemaphore image_available;
        VkFence in_flight;
    };
    GraphicsContext *const gc_ptr;
    CommandManager *const cmd_mg;
    std::vector<FrameData> frames;
    uint32_t current_index;
    uint32_t max_frames_in_flight;
};


