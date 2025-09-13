#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <chrono>
#include <memory>

#include "Camera.h"
#include "Scene.h"
#include "Shader.h"

class Renderer {
  public:
    Renderer();
    ~Renderer();

    bool initialize();
    void run();
    void shutdown();

  private:
    GLFWwindow* m_window;
    int         m_window_width  = 1200;
    int         m_window_height = 800;

    std::unique_ptr<Shader> m_voxel_shader;
    std::unique_ptr<Shader> m_compute_shader;
    std::unique_ptr<Shader> m_wireframe_shader;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Scene>  m_scene;

    std::chrono::high_resolution_clock::time_point m_last_frame;
    float                                          m_delta_time   = 0.0f;
    float                                          m_current_time = 0.0f;

    float                                          m_frame_time  = 0.0f;
    int                                            m_frame_count = 0;
    float                                          m_fps         = 0.0f;
    std::chrono::high_resolution_clock::time_point m_last_fps_update;

    int  m_render_frames = 0;
    bool m_camera_moving = false;

    bool  m_first_mouse = true;
    float m_last_x = 400, m_last_y = 300;
    bool  m_cursor_captured = true;
    bool  m_tab_key_pressed = false;

    bool m_wireframe_mode = false;
    bool m_bvh_wireframe  = false;
    bool m_vsync_enabled  = false;
    bool m_msaa_enabled   = true;

    bool initializeOpenGL();
    bool initializeImGui();

    void handleInput();
    void update();
    void render();
    void renderImGui();
    void updateFPS();
    void applyRenderSettings();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};
