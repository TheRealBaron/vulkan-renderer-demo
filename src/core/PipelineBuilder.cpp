#include "core/PipelineBuilder.hpp"
#include "resources/vertex_data.h"


PipelineBuilder::PipelineBuilder(GraphicsContext *const gcptr, Swapchain *const sc) 
        : gc_ptr(gcptr), sc_ptr(sc) {
    clear_and_setup_defaults();
}


void PipelineBuilder::clear_and_setup_defaults() {
    
    dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
    };

    unfixed_states = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    vertex_shader.reset(nullptr);

    viewport_scissor = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 0,
        .pViewports = nullptr,
        .scissorCount = 0,
        .pScissors = nullptr
    };
    
    rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_FRONT_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    }; 
    
    depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };
    
    fragment_shader.reset(nullptr);

    color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                          VK_COLOR_COMPONENT_G_BIT | 
                          VK_COLOR_COMPONENT_B_BIT | 
                          VK_COLOR_COMPONENT_A_BIT,
    };


    color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = { 0.f, 0.f, 0.f, 0.f }
    };

    descriptor_set_layouts.clear();
}


void PipelineBuilder::setup_input() {
    binding_descs = { Vertex::get_binding_description() };
    auto attrs = Vertex::get_attribute_descriptions();

    attr_descs.assign(attrs.begin(), attrs.end());

    input.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descs.size());
    input.pVertexBindingDescriptions = binding_descs.data();
    input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_descs.size());
    input.pVertexAttributeDescriptions = attr_descs.data();
}

void PipelineBuilder::setup_vertex_shader(const std::filesystem::path& path) {
    vertex_shader = std::make_unique<Shader>(gc_ptr, path, ShaderType::VERTEX);
}

void PipelineBuilder::setup_fragment_shader(const std::filesystem::path& path) {
    fragment_shader = std::make_unique<Shader>(gc_ptr, path, ShaderType::FRAGMENT);
}

void PipelineBuilder::build_pipeline(VkPipeline& pipeline, VkPipelineLayout& pipeline_layout) {
    VkDevice mydevice = gc_ptr->get_device();
    
    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    if (vkCreatePipelineLayout(mydevice, &pipeline_layout_ci, nullptr, &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    std::vector<VkPipelineShaderStageCreateInfo> shader_infos = {
        vertex_shader->get_stage_info(),
        fragment_shader->get_stage_info()
    };
    
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shader_infos.size()),
        .pStages = shader_infos.data(),
        .pVertexInputState = &input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_scissor,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &unfixed_states,
        .layout = pipeline_layout,
        .renderPass = sc_ptr->get_render_pass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(
        mydevice, 
        VK_NULL_HANDLE, 
        1, 
        &pipeline_info, 
        nullptr, 
        &pipeline) != VK_SUCCESS) {

        throw std::runtime_error("could not create graphics pipeline");
    }
}
