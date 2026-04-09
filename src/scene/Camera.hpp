#include "resources/UniformBuffer.hpp"


class Camera {
public:
    enum class Move {
        FORWARD, BACK, LEFT, RIGHT, UP, DOWN
    };
    
    Camera(
        GLFWwindow *const wnd,
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

    void update(size_t i);
    void update_t(float t);
    void move_event_callback(Move move, float d);
    void overlook_event_callback(float x_offset, float y_offset);

private:

    glm::vec3 pos;
    glm::vec2 facing;
    glm::mat4 proj;
    CameraBuffer camera_buffer;
    float last;
    float delta;
    float last_x;
    float last_y;
    
    void setup_input_callbacks(GLFWwindow *const window);

    friend void input_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    friend void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
};


/*  convert facing vector from spherical coordinates to cartecian 
 *  0' azimuth == +Z axis, small positive angles in direction +X
 *  0' polar == +Y axis, positive angles == up
 */
static glm::vec3 from_spherical_to_cartecian(const glm::vec2& azimuth_polar);

//get "right" direction relative to facing direction
static glm::vec3 get_right(const glm::vec3& facing);

