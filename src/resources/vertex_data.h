#pragma once
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "vulkan/vulkan.hpp"

struct Vertex {
    glm::vec3 pos;

    static VkVertexInputBindingDescription get_binding_description() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static std::array<VkVertexInputAttributeDescription, 1> get_attribute_descriptions() {
         std::array<VkVertexInputAttributeDescription, 1> res = {
            VkVertexInputAttributeDescription {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            }
        };
        return res;
    }
};


const float a = (glm::sqrt(5.f) + 1.f) * 0.5f ;
inline float verticies[] = {
    a, 1., 0.,
    a, -1., 0.,
    -a, -1., 0.,
    -a, 1., 0.,

    1., 0., a,
    -1., 0., a,
    -1., 0., -a,
    1., 0., -a,

    0., a, 1.,
    0., a, -1.,
    0., -a, -1.,
    0., -a, 1
};

// inline int indicies[]{
//     // Ring 1 - around the top
//     4, 8, 5,   // was 4,5,8
//     8, 0, 4,   // was 4,8,0
//     0, 1, 4,   // was 4,0,1
//     1, 11, 4,  // was 4,1,11
//     11, 5, 4,  // was 4,11,5
// 
//     // Ring 1 connections
//     5, 8, 3,   // was 5,8,3
//     8, 9, 0,   // was 8,0,9
//     0, 7, 1,   // was 0,1,7
//     1, 10, 11, // was 1,11,10
//     11, 2, 5,  // was 11,5,2
// 
//     // Ring 2 - around the bottom
//     3, 9, 8,   // was 3,9,8
//     9, 7, 0,   // was 9,7,0
//     7, 10, 1,  // was 7,10,1
//     10, 2, 11, // was 10,2,11
//     2, 3, 5,   // was 2,3,5
//     
//     // Bottom cap
//     3, 6, 9,   // was 3,9,6
//     9, 6, 7,   // was 9,7,6
//     7, 6, 10,  // was 7,10,6
//     10, 6, 2,  // was 10,2,6
//     2, 6, 3,   // was 2,3,6
// };

// inline int indicies[]{
//     // Original: 4, 5, 8 (clockwise) -> Reverse to counter-clockwise
//     4, 8, 5,
//     4, 0, 8,
//     4, 1, 0,
//     4, 11, 1,
//     4, 5, 11,
// 
//     5, 3, 8,
//     8, 9, 0,
//     0, 7, 1,
//     1, 10, 11,
//     11, 2, 5,
// 
//     3, 8, 9,
//     9, 0, 7,
//     7, 1, 10,
//     10, 11, 2,
//     2, 5, 3,
//     
//     3, 6, 9,
//     9, 6, 7,
//     7, 6, 10,
//     10, 6, 2,
//     2, 6, 3,
// };

inline int indicies[]{
    4, 5, 8,
    4, 8, 0,
    4, 0, 1,
    4, 1, 11,
    4, 11, 5,

    5, 8, 3,
    8, 0, 9,
    0, 1, 7,
    1, 11, 10,
    11, 5, 2,

    3, 9, 8,
    9, 7, 0,
    7, 10, 1,
    10, 2, 11,
    2, 3, 5,
    
    3, 9, 6,
    9, 7, 6,
    7, 10, 6,
    10, 2, 6,
    2, 3, 6,
};


