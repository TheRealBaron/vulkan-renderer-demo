#include <filesystem>
#include "GraphicsContext.hpp"


enum class ShaderType {
    VERTEX,
    FRAGMENT
};


class Shader {
public:
    Shader(
        GraphicsContext* const cptr,
        const std::filesystem::path& path, 
        ShaderType shader_type
    );
    ~Shader();
    
    VkPipelineShaderStageCreateInfo get_stage_info();
private:
    GraphicsContext* const gc_ptr;
    VkShaderModule inst;
    VkShaderStageFlagBits stage;
    
    void read_bytes(const std::filesystem::path& filename, std::vector<uint32_t>& buffer);
};

