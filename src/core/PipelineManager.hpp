#pragma once
#include <string_view>
#include <unordered_map>

#include "core/PipelineBuilder.hpp"



class PipelineManager {
public:
    
    PipelineManager(VkDevice dev) 
        : pipelines(std::unordered_map<std::string_view, Pipeline>()), device(dev) {};
    ~PipelineManager();

    void add_pipeline(const std::string_view name, PipelineBuilder& bld);
    
    VkPipeline get_pipeline(const std::string_view name);

private:
    VkDevice device;
    struct Pipeline {
        VkPipeline inst; 
        VkPipelineLayout layout;
    };
    std::unordered_map<std::string_view, Pipeline> pipelines; 
};


