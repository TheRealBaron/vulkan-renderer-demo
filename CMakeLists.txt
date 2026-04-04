cmake_minimum_required(VERSION 3.20)
project(VulkanHelloWorld)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(tinyobjloader REQUIRED)


add_subdirectory(data)
add_subdirectory(src)



