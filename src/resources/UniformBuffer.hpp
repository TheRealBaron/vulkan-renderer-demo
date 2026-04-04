#pragma once
#include <type_traits>

#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

template <typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;


template<trivially_copyable T>
class UniformBuffer {
public:
    using Ubo = typename T;

    UniformBuffer(GraphicsContext *const gc, size_t frames_in_flight);

    void update(Ubo&& ubo, size_t i);
    void load();
    void unload();
    VkDescriptorBufferInfo get_desc_write(size_t i);

private:
    GraphicsContext *const gc;
    std::vector<VmaAllocation> allocs;
    std::vector<VmaAllocationInfo> infos;
    std::vector<VkBuffer> bufs;
};


struct CamUbo {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 view_pos;
    float padding;
};


struct MeshUbo {
    glm::mat4 loc;
    glm::mat4 rot;
    glm::mat4 scl;
};


struct LightingUbo {
    std::array<glm::vec4, 3> positions;
    float shiningness;
    float ambient;
    uint32_t use_blinn_phong;
    float unused[3];
};

using CameraBuffer = UniformBuffer<CamUbo>;
using TransformBuffer = UniformBuffer<MeshUbo>;
using LightingBuffer = UniformBuffer<LightingUbo>;


