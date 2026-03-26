#include "core/Shader.hpp"
#include <fstream>


Shader::Shader(
    GraphicsContext* const cptr,
    const std::filesystem::path& path, 
    ShaderType shader_type) : gc_ptr(cptr) {

    VkDevice mydevice = gc_ptr->get_device();
    
    switch (shader_type) {
        case ShaderType::VERTEX:
            stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case ShaderType::FRAGMENT:
            stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
    }


    std::vector<uint32_t> src;
    read_bytes(path, src);

    VkShaderModuleCreateInfo module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = src.size() * sizeof(uint32_t),
        .pCode = src.data()
    };
    if (vkCreateShaderModule(mydevice, &module_info, nullptr, &inst) != VK_SUCCESS) {
        throw std::runtime_error("could not create shader module");
    }
}


Shader::~Shader() {
    vkDestroyShaderModule(gc_ptr->get_device(), inst, nullptr);
}


VkPipelineShaderStageCreateInfo Shader::get_stage_info() {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = inst,
        .pName = "main",
    };
}


void Shader::read_bytes(const std::filesystem::path& spirv_path, std::vector<uint32_t>& buffer) {
    std::ifstream file(spirv_path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader binary");
    }
    
    size_t fsize = static_cast<size_t>(file.tellg());
    
    if (fsize % 4 != 0) {
        throw std::runtime_error("the spir-v binary size is != 0 (mod 4)");
    }

    buffer.resize(fsize / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fsize);
    file.close();
}

