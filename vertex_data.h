#pragma once
#include <array>
#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription get_binding_description() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() {
         std::array<VkVertexInputAttributeDescription, 2> res = {
            VkVertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            },
            VkVertexInputAttributeDescription {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color)
            }
        };
        return res;
    }
};


const std::array<Vertex, 6> vertices = {
    Vertex{glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}, 
    Vertex{glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f)}, 
    Vertex{glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)},   
    
    Vertex{glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)},   
    Vertex{glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 0.0f)},  
    Vertex{glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)}  
};

