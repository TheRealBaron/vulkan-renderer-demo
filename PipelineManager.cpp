#include "PipelineManager.hpp"

PipelineManager::~PipelineManager() {

    for (auto& item : pipelines) {
        Pipeline& pl = item.second;
        
        vkDestroyPipeline(device, pl.inst, nullptr);
        vkDestroyPipelineLayout(device, pl.layout, nullptr);
    }

}

void PipelineManager::add_pipeline(const std::string_view name, PipelineBuilder& bld) {
    
    if (pipelines.contains(name)) {
        throw std::runtime_error("name for pipeline is already in use");
    }

    pipelines[name] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
    Pipeline& new_pipeline = pipelines[name];
    
    bld.build_pipeline(new_pipeline.inst, new_pipeline.layout);
}

VkPipeline PipelineManager::get_pipeline(const std::string_view name) {
    return pipelines[name].inst;
}

