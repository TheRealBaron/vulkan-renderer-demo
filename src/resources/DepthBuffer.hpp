#pragma once
#include "core/GraphicsContext.hpp"
#include "core/CommandManager.hpp"


class DepthBuffer {
public:
    DepthBuffer(GraphicsContext *const gcptr);
    
    inline VkImageView get_view() { return image_view; }

    void load(VkFormat depth_format, VkExtent2D extent);
    void unload();

private:
    GraphicsContext *const gc_ptr;
    VmaAllocation allocation;
    VkImage image;
    VkImageView image_view;
};

