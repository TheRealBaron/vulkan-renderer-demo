#include "scene/Camera.hpp"
#include "utils/logger.hpp"


Camera::Camera(GraphicsContext *const gc, size_t buf_cnt, glm::vec3 pos, glm::vec2 facing_angles, float aspect): 
    pos(pos), 
    facing(facing_angles),
    proj(std::move(glm::perspectiveRH_ZO(3.14159f / 3, aspect, 0.1f, 25.f))),
    camera_buffer(gc, buf_cnt) {
    
    proj[1][1] *= -1.f;

    camera_buffer.load();
    
    for (size_t i = 0; i < buf_cnt; ++i) {
        update(i);
    }
}

Camera::~Camera() {
    camera_buffer.unload();
}


void Camera::update(size_t i) {

    logger::log(LStatus::INFO, "facing: azimuth={}, polar={}", facing.x, facing.y);
    
    glm::vec3 facing_dir(
        glm::sin(glm::radians(facing.x)) * glm::cos(glm::radians(facing.y)),
        glm::sin(glm::radians(facing.y)),
        glm::cos(glm::radians(facing.x)) * glm::cos(glm::radians(facing.y))
    );
    
    glm::vec3 world_up(0.f, 1.f, 0.f);
    glm::mat4 view = glm::lookAtRH(pos, pos + facing_dir, world_up);
    
    camera_buffer.update(
        CameraBuffer::Ubo {
            .view = view,
            .proj = proj,
            .view_pos = pos
        },
        i
    );
}

