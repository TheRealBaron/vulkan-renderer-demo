#include <vector>
#include <array>
#include <algorithm>

#include "core/Renderer.hpp"
#include "scene/Scene.hpp"
#include "myapp.h"
#include "utils/logger.hpp"

// common objects
GLFWwindow* window;
std::unique_ptr<Renderer> renderer;
std::unique_ptr<Scene> scene;


void myapp::run() {
    myapp::initWindow();
    myapp::initVulkan();
    myapp::mainloop();
    myapp::cleanup();
}


static void myapp::initWindow() {
    if (!glfwInit()) {
        throw std::exception("could not initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    window = glfwCreateWindow(mode->width, mode->height, "demo", nullptr, nullptr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

}


static void myapp::initVulkan() {

    renderer = std::make_unique<Renderer>(window);
    logger::log(LStatus::INFO, "successfully initialized renderer");
    
    scene = std::make_unique<Scene>(
        window,
        renderer->get_context(),
        renderer->get_swapchain(),
        renderer->get_command_manager()
    );
    logger::log(LStatus::INFO, "successfully initialized scene with single mesh");
    
    renderer->prepare_for_rendering(scene.get());
    
    logger::log(LStatus::INFO, "renderer is ready to draw a scene");

    glfwShowWindow(window);
    glfwMakeContextCurrent(window);
}


static void myapp::mainloop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer->draw_frame(scene.get());
    }
}


static void myapp::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

