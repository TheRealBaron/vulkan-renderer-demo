#pragma once
#include <algorithm>
#include "core/GraphicsContext.hpp"
#include "core/Swapchain.hpp"
#include "core/Shader.hpp"


class PipelineBuilder {
public:
    PipelineBuilder(GraphicsContext *const gcptr, Swapchain *const sc);

    void clear_and_setup_defaults();
    void setup_input();
    void setup_vertex_shader(const std::filesystem::path& path);
    void setup_fragment_shader(const std::filesystem::path& path);

    template <std::ranges::input_range R>
    inline void setup_layout(R&& r) {
        descriptor_set_layouts.assign(
            r.begin(), r.end()
        );
    }

    
private:
    void build_pipeline(VkPipeline& pipeline, VkPipelineLayout& pipeline_layout);
    
    GraphicsContext *const gc_ptr;
    Swapchain       *const sc_ptr;
    
    std::vector<VkDynamicState> dynamic_states;
    VkPipelineDynamicStateCreateInfo unfixed_states;
    
    std::vector<VkVertexInputBindingDescription> binding_descs;
    std::vector<VkVertexInputAttributeDescription> attr_descs;
    VkPipelineVertexInputStateCreateInfo input;  //customizable
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    
    std::unique_ptr<Shader> vertex_shader; //customizable
    VkPipelineViewportStateCreateInfo viewport_scissor;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
    std::unique_ptr<Shader> fragment_shader; // customizable
    VkPipelineColorBlendAttachmentState color_blend_attachment;
    VkPipelineColorBlendStateCreateInfo color_blend_state;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts; //customizable

    friend class PipelineManager;
};

