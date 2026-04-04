#include "resources/ReadonlyBuffer.hpp"


ReadonlyBuffer::ReadonlyBuffer(GraphicsContext *const gcptr, CommandManager *const cmdmgptr) 
    : gc_ptr(gcptr), cmdmg_ptr(cmdmgptr){}

void ReadonlyBuffer::unload() { 
    vmaDestroyBuffer(gc_ptr->get_allocator(), buffer, allocation);
}


void ReadonlyBuffer::load(const void *const data, size_t size, Usage usage) {
    bufsize = size;
    VmaAllocator allocator = gc_ptr->get_allocator();
    VkBuffer tmp_buf;
    VmaAllocation tmp_alloc;
    VmaAllocationInfo tmp_alloc_info;
    
    VkBufferCreateInfo buf_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<uint32_t>(size),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST
    };
    if (vmaCreateBuffer(allocator, &buf_ci, &alloc_ci, &tmp_buf, &tmp_alloc, &tmp_alloc_info) != VK_SUCCESS) {
        throw std::runtime_error("could not create and allocate tmp buffer");
    }

    buf_ci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    switch (usage) {
        case Usage::VERTEX:
            buf_ci.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case Usage::INDEX:
            buf_ci.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
    }

    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    alloc_ci.flags = 0x0;

    if (vmaCreateBuffer(allocator, &buf_ci, &alloc_ci, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("could not create and allocate buffer");
    }

    memcpy(tmp_alloc_info.pMappedData, data, size);
    
    copy_buffer_contents(tmp_buf, buffer, static_cast<VkDeviceSize>(size));

    vmaDestroyBuffer(allocator, tmp_buf, tmp_alloc);
}

void ReadonlyBuffer::copy_buffer_contents(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    
    VkCommandBuffer cmdbuf = cmdmg_ptr->allocate_transfer();
    VkDevice device = gc_ptr->get_device();
    VkQueue transfer_q = gc_ptr->get_transfer_queue();
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdbuf
    };
    VkCommandBufferBeginInfo cmd_bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkBufferCopy copy_region = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkBeginCommandBuffer(cmdbuf, &cmd_bi);
    vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copy_region);
    vkEndCommandBuffer(cmdbuf);

    vkQueueSubmit(transfer_q, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_q);
    
    cmdmg_ptr->destroy_transfer(cmdbuf);
}

