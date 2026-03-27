#pragma once
#include "core/GraphicsContext.hpp"
#include "core/CommandManager.hpp"


class ReadonlyBuffer {
public:
    enum class Usage {
        VERTEX,
        INDEX
    };
    ReadonlyBuffer(GraphicsContext *const gcptr, CommandManager *const cmdmgptr);

    void load(const void *const data, size_t size, Usage usage);
    void unload();
    inline VkBuffer get_h() { return buffer; }

private:
    GraphicsContext *const gc_ptr;
    CommandManager *const cmdmg_ptr;
    VmaAllocation allocation;
    VkBuffer buffer;

    void copy_buffer_contents(VkBuffer src, VkBuffer dst, VkDeviceSize size);
};


