cmake_minimum_required(VERSION 3.16)
project(VoxRay)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_library(glad libs/glad/src/glad.c libs/glad/include/glad/glad.h)
target_include_directories(glad PUBLIC libs/glad/include)

file(GLOB IMGUI_SOURCES 
    libs/imgui/*.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC libs/imgui libs/imgui/backends)
target_link_libraries(imgui glfw)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "src/*.hpp")
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    libs/glad/include
    libs/imgui
    libs/imgui/backends
    libs/tinybvh
    libs/glm
)

target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL 
    glfw 
    glad 
    imgui
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
endif()
