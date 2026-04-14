#include "scene/Scene.hpp"
#include "animations/animation.hpp"


Scene::Scene(GLFWwindow *const window, GraphicsContext *const gc, Swapchain *const sc, CommandManager *const cmdmg) 
    : gc(gc), sc(sc), cmdmg(cmdmg) {

    mesh = std::make_unique<Mesh>(gc, cmdmg, sc->get_images_cnt(), "data/pes.obj");
    float ang_h = glm::radians(15.f);
    float ang_v = glm::radians(22.5f);
    
    camera = std::make_unique<Camera>(
        window,
        gc, 
        sc->get_images_cnt(),
        glm::vec3(0.f, glm::sin(ang_v), -glm::cos(ang_h)) * 22.f,
        glm::vec2(0.f, -22.5f),
        
        sc->get_aspect()
    );
    
    create_descriptor_set_layout();
    create_descriptor_pool();
    create_descriptor_sets();
}


Scene::~Scene() {
    vkDeviceWaitIdle(gc->get_device());
    vkDestroyDescriptorPool(gc->get_device(), descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(gc->get_device(), layout, nullptr);
}


void Scene::configure_pipeline_builder(PipelineBuilder *const pipeline_builder) {
    
    pipeline_builder->clear_and_setup_defaults();
    pipeline_builder->setup_input();
    pipeline_builder->setup_vertex_shader("shaders/vertex.spv");
    pipeline_builder->setup_fragment_shader("shaders/fragment.spv");
    
    std::array<VkDescriptorSetLayout, 1> layouts = { layout };
    pipeline_builder->setup_layout(layouts);
    
}

void Scene::record_draw_commands(VkCommandBuffer buf, VkPipelineLayout pipeline_layout, size_t i) {

    VkBuffer vertex_bufs[] = { mesh->get_vertex_h() };
    VkBuffer index_buf = mesh->get_index_h();
    VkDeviceSize offsets[] = { 0 };

    
    vkCmdBindVertexBuffers(buf, 0, 1, vertex_bufs, offsets);
    vkCmdBindIndexBuffer(buf, index_buf, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(
        buf, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr
    );
    
    vkCmdDrawIndexed(buf, mesh->get_indices_cnt(), 1, 0, 0, 0);
}

void Scene::create_descriptor_set_layout() {
    
    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { 
        VkDescriptorSetLayoutBinding { /* Camera data */
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT

        },
        VkDescriptorSetLayoutBinding { /* Mesh transform data */
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        },
        VkDescriptorSetLayoutBinding { /* Lighting data */
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        }
    };

    VkDescriptorSetLayoutCreateInfo dset_layout_ci =  {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    
    if (vkCreateDescriptorSetLayout(gc->get_device(), &dset_layout_ci, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("could not create descriptor set layout");
    }
    logger::log(LStatus::INFO, "created descriptor set layout");

}


void Scene::create_descriptor_pool() {
    VkDevice device = gc->get_device();

    VkDescriptorPoolSize cnt = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 16
    };
    VkDescriptorPoolCreateInfo desc_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 8,
        .poolSizeCount = 1,
        .pPoolSizes = &cnt
    };
    if (vkCreateDescriptorPool(device, &desc_pool_ci, nullptr, &descriptor_pool) != VK_SUCCESS) {
        throw std::runtime_error("could not create descriptor pool");
    }
    logger::log(LStatus::INFO, "created descriptor pool");
}


void Scene::create_descriptor_sets() {
    size_t cnt = sc->get_images_cnt();
    VkDevice device = gc->get_device();
    
    descriptor_sets.resize(cnt);
    std::vector<VkDescriptorSetLayout> layouts(cnt, layout);
    VkDescriptorSetAllocateInfo descriptor_sets_ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = static_cast<uint32_t>(cnt),
        .pSetLayouts = layouts.data()
    };
    if (vkAllocateDescriptorSets(device, &descriptor_sets_ai, descriptor_sets.data()) != VK_SUCCESS) {
        throw std::runtime_error("could not allocate descriptor sets");
    }
    logger::log(LStatus::INFO, "allocated descriptor sets");

    for (size_t i = 0; i < descriptor_sets.size(); ++i) {
        VkDescriptorBufferInfo camera_info = camera->get_desc_write(i);
        VkDescriptorBufferInfo transform_info = mesh->get_desc_write(i);

        std::array<VkWriteDescriptorSet, 2> write_info = {
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &camera_info
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &transform_info
            }
        };

        vkUpdateDescriptorSets(
            device,
            static_cast<uint32_t>(write_info.size()),
            write_info.data(),
            0,
            nullptr
        );
    }
    logger::log(LStatus::INFO, "updated descriptor sets");
}


void Scene::update(float t, size_t buf_i) {
    glm::mat4 scl(0.1f);
    scl[3][3] = 1.f;
    mesh->update_ubo(
        TransformBuffer::Ubo {
            animate::drammaticMovement(t),
            animate::drammaticRotation(t),
            scl
        },
        buf_i
    );
    camera->update_t(t);
    camera->update(buf_i);
    
}


