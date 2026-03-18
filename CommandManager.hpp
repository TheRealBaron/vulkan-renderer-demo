#pragma once
#include "GraphicsContext.hpp"


class CommandManager {
public:
    CommandManager(GraphicsContext *const gcptr);
    ~CommandManager();

    VkCommandBuffer allocate_graphics();
    VkCommandBuffer allocate_transfer();
    void destroy_graphics(VkCommandBuffer buf);
    void destroy_transfer(VkCommandBuffer buf);

private:
    GraphicsContext *const gc_ptr;
    VkCommandPool graphics_cmd_buf_pool;
    VkCommandPool transfer_cmd_buf_pool;
};

