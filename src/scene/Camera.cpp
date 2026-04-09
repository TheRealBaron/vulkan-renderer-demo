#include "scene/Camera.hpp"
#include "utils/logger.hpp"


void input_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);

Camera::Camera(GLFWwindow *const wnd, GraphicsContext *const gc, size_t buf_cnt, glm::vec3 pos, glm::vec2 facing_angles, float aspect): 
    pos(pos), 
    facing(facing_angles),
    proj(std::move(glm::perspectiveRH_ZO(3.14159f / 3, aspect, 0.1f, 25.f))),
    camera_buffer(gc, buf_cnt), last(0.f) {
    
    proj[1][1] *= -1.f;
    
    setup_input_callbacks(wnd);

    camera_buffer.load();
    
    for (size_t i = 0; i < buf_cnt; ++i) {
        update(i);
    }
}

Camera::~Camera() {
    camera_buffer.unload();
}


void Camera::update(size_t i) {

    glm::vec3 facing_dir = from_spherical_to_cartecian(facing);
    
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

void Camera::update_t(float t) {
    delta = std::abs(t - last);
    last = t;
}


void Camera::move_event_callback(Move move, float d) {
    constexpr float SPEED = 0.1f;
    glm::vec3 c_facing = from_spherical_to_cartecian(facing);
    glm::vec3 right = get_right(c_facing);
	glm::mat3x3 splash_xz(
		1.f, 0.f, 0.f,
		0.f, 0.f, 0.f,
		0.f, 0.f, 1.f
	);
    
    glm::vec3 forward = glm::normalize(c_facing * splash_xz);

    switch (move) {
        case Move::FORWARD:
            pos += forward * SPEED;
            break;
        case Move::BACK:
            pos -= forward * SPEED;
            break;
        case Move::RIGHT:
            pos += right * SPEED;
            break;
        case Move::LEFT:
            pos -= right * SPEED;
            break;
        case Move::UP:
            pos.y += SPEED;
            break;
        case Move::DOWN:
            pos.y -= SPEED;
            break;
    }
    
}

void Camera::overlook_event_callback(float x_offset, float y_offset) {
    constexpr float SENSIVITY = 0.01f;
    x_offset *= SENSIVITY;
    y_offset *= SENSIVITY;
    
    facing.x += x_offset;
    facing.y = std::clamp(facing.y + y_offset, -89.f, 89.f);
}


void Camera::setup_input_callbacks(GLFWwindow *const window) {
    
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    last_x = static_cast<float>(w) / 2;
    last_y = static_cast<float>(h) / 2;

    
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, input_callback);
    glfwSetCursorPosCallback(window, mouse_move_callback);

}



void input_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    
    Camera *const cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (key == GLFW_KEY_W) {
            cam->move_event_callback(Camera::Move::FORWARD, cam->delta);
        }
        if (key == GLFW_KEY_S) {
            cam->move_event_callback(Camera::Move::BACK, cam->delta);
        }
        if (key == GLFW_KEY_A) {
            cam->move_event_callback(Camera::Move::LEFT, cam->delta);
        }
        if (key == GLFW_KEY_D) {
            cam->move_event_callback(Camera::Move::RIGHT, cam->delta);
        }
        if (key == GLFW_KEY_SPACE) {
            cam->move_event_callback(Camera::Move::UP, cam->delta);
        }
        if (key == GLFW_KEY_LEFT_SHIFT) {
            cam->move_event_callback(Camera::Move::DOWN, cam->delta);
        }
    }
}


void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
    Camera *const cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    float offset_x = cam->last_x - static_cast<float>(xpos);
    float offset_y = cam->last_y - static_cast<float>(ypos);
    cam->overlook_event_callback(offset_x, offset_y);
    cam->last_x = static_cast<float>(xpos);
    cam->last_y = static_cast<float>(ypos);
}
 

static glm::vec3 from_spherical_to_cartecian(const glm::vec2& ap) {
    return glm::vec3(
        glm::sin(glm::radians(ap.x)) * glm::cos(glm::radians(ap.y)),
        glm::sin(glm::radians(ap.y)),
        glm::cos(glm::radians(ap.x)) * glm::cos(glm::radians(ap.y))
    );
}


static glm::vec3 get_right(const glm::vec3& facing) {
    glm::vec3 up(0.f, 1.f, 0.f);
    return glm::normalize(
        glm::cross(facing, up)
    );
}

