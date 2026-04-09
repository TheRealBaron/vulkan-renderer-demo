#pragma once
#include "utils/logger.hpp"
#include "core/GraphicsContext.hpp"
#include "core/CommandManager.hpp"
#include "core/PipelineBuilder.hpp"
#include "scene/Camera.hpp"
#include "scene/Mesh.hpp"


class Scene {
public:
    
    Scene(GLFWwindow *const window, GraphicsContext *const gc, Swapchain *const sc, CommandManager *const cmdmg);
    ~Scene();
    
    void configure_pipeline_builder(PipelineBuilder *const pipeline_builder);
    void record_draw_commands(VkCommandBuffer buf, VkPipelineLayout pipeline_layout, size_t i);
    void update(float t, size_t buf_i);

private:
    GraphicsContext *const gc;
    Swapchain       *const sc;
    CommandManager  *const cmdmg;

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Camera> camera;
    
    VkDescriptorSetLayout layout;
    VkDescriptorPool descriptor_pool;
    std::vector<VkDescriptorSet> descriptor_sets;

    void create_descriptor_set_layout();

    void create_descriptor_pool();
    void create_descriptor_sets();
};


