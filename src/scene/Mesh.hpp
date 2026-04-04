#pragma once

#include "resources/UniformBuffer.hpp"
#include "resources/ReadonlyBuffer.hpp"
#include <filesystem>


class Mesh {
public:
    
    Mesh(GraphicsContext *const gc, CommandManager *const cmdmg, size_t bufcnt, std::filesystem::path obj_path);
    ~Mesh();
    
    inline VkDescriptorBufferInfo get_desc_write(size_t i) {
        return transform_buffer.get_desc_write(i);
    }

    inline uint32_t get_indices_cnt() {
        return index_buffer.get_u32_cnt();
    }
    
    inline void update_ubo(TransformBuffer::Ubo&& ubo, size_t i) {
        transform_buffer.update(std::move(ubo), i);
    }
    
    inline VkBuffer get_vertex_h() { return vertex_buffer.get_h(); }
    inline VkBuffer get_index_h() { return index_buffer.get_h(); }

private:

    static void load_data(
        std::filesystem::path obj_path, 
        std::vector<uint8_t>& vertex_data, 
        std::vector<uint8_t>& index_data
    );
    
    ReadonlyBuffer vertex_buffer;
    ReadonlyBuffer index_buffer;
    TransformBuffer transform_buffer;
};


