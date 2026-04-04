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
    inline uint32_t get_u32_cnt() { return bufsize / 4; }

private:
    uint32_t bufsize;
    GraphicsContext *const gc_ptr;
    CommandManager *const cmdmg_ptr;
    VmaAllocation allocation;
    VkBuffer buffer;

    void copy_buffer_contents(VkBuffer src, VkBuffer dst, VkDeviceSize size);
};


