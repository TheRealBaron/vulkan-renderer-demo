#include "resources/UniformBuffer.hpp"



template<trivially_copyable T>
UniformBuffer<T>::UniformBuffer(GraphicsContext *const gc, size_t frames_in_flight) :
        gc(gc),
        allocs(frames_in_flight, VK_NULL_HANDLE),
        infos(frames_in_flight, VmaAllocationInfo{}),
        bufs(frames_in_flight, VK_NULL_HANDLE) {

}


template<trivially_copyable T>
void UniformBuffer<T>::load() { 

    VmaAllocator allocator = gc->get_allocator();

    VkBufferCreateInfo buf_ci =  {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<uint32_t>(sizeof(Ubo)),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    
    for (size_t i = 0; i < bufs.size(); ++i) {
        
        VkResult res = vmaCreateBuffer(
            allocator, 
            &buf_ci, 
            &alloc_ci, 
            bufs.data() + i, 
            allocs.data() + i,
            infos.data() + i
        );
        if (res != VK_SUCCESS) {
            throw std::runtime_error("could not create uniform buffer");
        }
        update(Ubo{}, i);
        
    }

}



template<trivially_copyable T>
void UniformBuffer<T>::unload() { 
    VmaAllocator allocator = gc->get_allocator();
    for (size_t i = 0; i < bufs.size(); ++i) {
        vmaDestroyBuffer(allocator, bufs[i], allocs[i]);
    }
}


template<trivially_copyable T>
void UniformBuffer<T>::update(Ubo&& ubo, size_t i) {
    Ubo tmp_ubo = ubo;
    memcpy(infos[i].pMappedData, static_cast<void*>(&ubo), sizeof(Ubo));
}

template<trivially_copyable T>
VkDescriptorBufferInfo UniformBuffer<T>::get_desc_write(size_t i) {
    return VkDescriptorBufferInfo {
        .buffer = bufs[i],
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };
}


template class UniformBuffer<CamUbo>;
template class UniformBuffer<MeshUbo>;
template class UniformBuffer<LightingUbo>;


