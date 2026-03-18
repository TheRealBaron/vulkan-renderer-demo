#include "CommandManager.hpp"

CommandManager::CommandManager(GraphicsContext *const gcptr) : gc_ptr(gcptr) {
    
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = gc_ptr->get_transfer_family_index()
    };
    
    if (vkCreateCommandPool(gc_ptr->get_device(), &create_info, nullptr, &transfer_cmd_buf_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create command pool");
    }

    create_info.queueFamilyIndex = gc_ptr->get_graphics_family_index();
    if (vkCreateCommandPool(gc_ptr->get_device(), &create_info, nullptr, &graphics_cmd_buf_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create command pool");
    }
}


CommandManager::~CommandManager() {
    vkDestroyCommandPool(gc_ptr->get_device(), graphics_cmd_buf_pool, nullptr);
    vkDestroyCommandPool(gc_ptr->get_device(), transfer_cmd_buf_pool, nullptr);
}


VkCommandBuffer CommandManager::allocate_graphics() {
    VkDevice device = gc_ptr->get_device();
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = graphics_cmd_buf_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer cmdbuffer;
    if (vkAllocateCommandBuffers(device, &alloc_info, &cmdbuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create command buffer for graphics");
    }
    return cmdbuffer;
}


VkCommandBuffer CommandManager::allocate_transfer() {
    VkDevice device = gc_ptr->get_device();
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = transfer_cmd_buf_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer cmdbuffer;
    if (vkAllocateCommandBuffers(device, &alloc_info, &cmdbuffer) != VK_SUCCESS) {
        throw std::runtime_error("could not create command buffer for transfer");
    }
    return cmdbuffer;
}

void CommandManager::destroy_graphics(VkCommandBuffer buf) {
    vkFreeCommandBuffers(
        gc_ptr->get_device(),
        graphics_cmd_buf_pool,
        1,
        &buf
    );
}

void CommandManager::destroy_transfer(VkCommandBuffer buf) {
    vkFreeCommandBuffers(
        gc_ptr->get_device(),
        transfer_cmd_buf_pool,
        1,
        &buf
    );
}
