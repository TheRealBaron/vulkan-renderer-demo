#pragma once
#include <type_traits>

#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"
#include "glm/glm.hpp"


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
    glm::mat4 view;
    glm::mat4 proj;
};

struct MeshUbo {
    glm::mat4 loc;
    glm::mat4 rot;
};

using CameraBuffer = UniformBuffer<CamUbo>;
using TransformBuffer = UniformBuffer<MeshUbo>;

