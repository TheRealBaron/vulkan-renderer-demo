#include "resources/DepthBuffer.hpp"

DepthBuffer::DepthBuffer(GraphicsContext *const gcptr) : gc_ptr(gcptr) {}


void DepthBuffer::load(VkFormat depth_format, VkExtent2D extent) {
    VmaAllocator allocator = gc_ptr->get_allocator();
    VkDevice device = gc_ptr->get_device();

    VkImageCreateInfo depth_image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    if (vmaCreateImage(allocator, &depth_image_ci, &alloc_ci, &image, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate depth buffer");
    }
    
    VkImageViewCreateInfo depth_view_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
    };

    if (vkCreateImageView(device, &depth_view_ci, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("could not create depth image view");
    }
}

void DepthBuffer::unload() {
    vkDestroyImageView(gc_ptr->get_device(), image_view, nullptr);
    vmaDestroyImage(gc_ptr->get_allocator(), image, allocation);
}


