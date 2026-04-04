#include "resources/UniformBuffer.hpp"


class Camera {
public:
    
    Camera(
        GraphicsContext *const gc, 
        size_t buf_cnt, 
        glm::vec3 pos, 
        glm::vec2 facing_angles,
        float aspect
    );
    ~Camera();
    
    inline VkDescriptorBufferInfo get_desc_write(size_t i) {
        return camera_buffer.get_desc_write(i);
    }

    void shift_pos(glm::vec3&& d);
    void shift_facing(float azimuth_d, float polar_d);
    void update(size_t i); 

private:
    glm::vec3 pos;
    glm::vec2 facing;
    glm::mat4 proj;
    CameraBuffer camera_buffer;

};

