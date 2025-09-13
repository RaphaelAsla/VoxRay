#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Renderer::Renderer() : m_window(nullptr) {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    if (!initializeOpenGL()) {
        return false;
    }

    if (!initializeImGui()) {
        return false;
    }

    m_camera = std::make_unique<Camera>(glm::vec3(100.0f, 220.0f, 100.0f), glm::vec3(0.0f, 1.0f, 0.0f), -45.0f, -15.0f);
    m_scene  = std::make_unique<Scene>();
    m_scene->initialize(400, 400, 400);

    m_voxel_shader     = std::make_unique<Shader>("../shaders/voxel.vert", "../shaders/voxel.frag");
    m_compute_shader   = std::make_unique<Shader>("../shaders/voxel_compute.comp");
    m_wireframe_shader = std::make_unique<Shader>("../shaders/bvh_wireframe.vert", "../shaders/bvh_wireframe.frag");

    if (!m_voxel_shader->isValid()) {
        std::cerr << "Failed to load voxel shader" << std::endl;
        return false;
    }

    if (!m_compute_shader->isValid()) {
        std::cerr << "Failed to load compute shader" << std::endl;
        return false;
    }

    if (!m_wireframe_shader->isValid()) {
        std::cerr << "Failed to load wireframe shader" << std::endl;
        return false;
    }

    m_last_frame      = std::chrono::high_resolution_clock::now();
    m_last_fps_update = m_last_frame;

    std::cout << "Renderer initialized successfully!" << std::endl;
    return true;
}

bool Renderer::initializeOpenGL() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(m_window_width, m_window_height, "VoxRay", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW m_window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);

    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glfwSwapInterval(m_vsync_enabled ? 1 : 0);

    return true;
}

bool Renderer::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true)) {
        std::cerr << "Failed to initialize ImGui GLFW" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 430")) {
        std::cerr << "Failed to initialize ImGui OpenGL3" << std::endl;
        return false;
    }

    return true;
}

void Renderer::run() {
    while (!glfwWindowShouldClose(m_window)) {
        auto current_frame = std::chrono::high_resolution_clock::now();
        m_delta_time       = std::chrono::duration<float>(current_frame - m_last_frame).count();
        m_last_frame       = current_frame;

        m_current_time += m_delta_time;

        updateFPS();
        handleInput();
        update();
        render();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Renderer::handleInput() {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }

    bool current_c_state = (glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS);
    if (current_c_state && !m_tab_key_pressed) {
        m_cursor_captured = !m_cursor_captured;
        glfwSetInputMode(m_window, GLFW_CURSOR, m_cursor_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

        m_first_mouse = true;
    }
    m_tab_key_pressed = current_c_state;

    // Must make seperate input class at some point
    m_camera_moving = false;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
        m_camera->processKeyboard(0, m_delta_time);
        m_camera_moving = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
        m_camera->processKeyboard(1, m_delta_time);
        m_camera_moving = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
        m_camera->processKeyboard(2, m_delta_time);
        m_camera_moving = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
        m_camera->processKeyboard(3, m_delta_time);
        m_camera_moving = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_camera->processKeyboard(4, m_delta_time);
        m_camera_moving = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_camera->processKeyboard(5, m_delta_time);
        m_camera_moving = true;
    }
}

void Renderer::update() {
    m_voxel_shader->reload();
    m_compute_shader->reload();
    m_wireframe_shader->reload();

    applyRenderSettings();
}

void Renderer::render() {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_compute_shader && m_compute_shader->isValid() && m_scene) {
        m_compute_shader->use();
        m_compute_shader->setFloat("uTime", m_current_time);
        m_compute_shader->setIVec3("uGridSize", m_scene->getGridWidth(), m_scene->getGridHeight(), m_scene->getGridDepth());
        m_compute_shader->setInt("uFrameCount", m_render_frames);
        glm::vec3 cam_pos = m_camera->getPosition();
        m_compute_shader->setVec3("uCameraPosition", cam_pos.x, cam_pos.y, cam_pos.z);
        m_compute_shader->setBool("uCameraMoving", m_camera_moving);

        int active_voxel_count = m_scene->getActiveVoxelCount();
        if (active_voxel_count > 0) {
            int local_size = 256;
            int groups     = (active_voxel_count + local_size - 1) / local_size;
            m_compute_shader->dispatch(groups, 1, 1);
        }
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        m_render_frames++;
    }

    glm::mat4 view       = m_camera->getViewMatrix();
    glm::mat4 projection = m_camera->getProjectionMatrix(static_cast<float>(m_window_width) / static_cast<float>(m_window_height));

    if (m_voxel_shader && m_voxel_shader->isValid() && m_scene) {
        m_voxel_shader->use();

        m_voxel_shader->setMat4("uView", glm::value_ptr(view));
        m_voxel_shader->setMat4("uProjection", glm::value_ptr(projection));

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_scene->getColorBuffer());
        m_scene->render(m_voxel_shader.get(), view, projection);
    }

    if (m_bvh_wireframe && m_wireframe_shader && m_wireframe_shader->isValid() && m_scene) {
        glDisable(GL_DEPTH_TEST);  // Must be infront of voxels
        m_scene->renderBVHWireframe(m_wireframe_shader.get(), view, projection);
        glEnable(GL_DEPTH_TEST);
    }

    renderImGui();
}

void Renderer::renderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("VoxRay Controls");

    ImGui::Text("FPS: %.1f (%.3f ms/frame)", m_fps, m_frame_time * 4000.0f);
    if (m_scene) {
        ImGui::Text("Active Voxels: %d", m_scene->getActiveVoxelCount());
        ImGui::Text("Rendered Faces: %d", m_scene->getFaceCount());
    }

    ImGui::Separator();

    ImGui::Text("Render Settings");

    if (ImGui::Checkbox("Voxel Wireframe", &m_wireframe_mode)) {
        applyRenderSettings();
    }

    ImGui::Checkbox("BVH Wireframe", &m_bvh_wireframe);

    if (ImGui::Checkbox("VSync", &m_vsync_enabled)) {
        glfwSwapInterval(m_vsync_enabled ? 1 : 0);
    }

    if (ImGui::Checkbox("MSAA", &m_msaa_enabled)) {
        if (m_msaa_enabled) {
            glEnable(GL_MULTISAMPLE);
        } else {
            glDisable(GL_MULTISAMPLE);
        }
    }

    ImGui::Separator();

    ImGui::Text("BVH Settings");
    if (m_scene) {
        ImGui::Text("BVH Nodes: %d", m_scene->getBVHNodeCount());
    }

    ImGui::Separator();

    ImGui::Text("Accumulation");
    ImGui::Text("Render Frames: %d", m_render_frames);
    if (ImGui::Button("Reset Accumulation")) {
        m_render_frames = 0;
        if (m_scene) {
            m_scene->clearAccumulation();
        }
    }

    ImGui::Separator();

    ImGui::Text("Camera");
    glm::vec3 cam_pos = m_camera->getPosition();
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", cam_pos.x, cam_pos.y, cam_pos.z);
    ImGui::Text("Cursor: %s", m_cursor_captured ? "Captured (Camera Control)" : "Free (ImGui Mode)");
    ImGui::Text("Press C to toggle cursor mode");

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::updateFPS() {
    m_frame_count++;
    auto  current_time = std::chrono::high_resolution_clock::now();
    float elapsed      = std::chrono::duration<float>(current_time - m_last_fps_update).count();

    if (elapsed >= 0.1f) {
        m_fps             = m_frame_count / elapsed;
        m_frame_time      = elapsed / m_frame_count;
        m_frame_count     = 0;
        m_last_fps_update = current_time;
    }
}

void Renderer::applyRenderSettings() {
    if (m_wireframe_mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}

void Renderer::framebufferSizeCallback(GLFWwindow* m_window, int width, int height) {
    Renderer* app        = static_cast<Renderer*>(glfwGetWindowUserPointer(m_window));
    app->m_window_width  = width;
    app->m_window_height = height;
    glViewport(0, 0, width, height);
}

void Renderer::mouseCallback(GLFWwindow* m_window, double xpos, double ypos) {
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(m_window));

    if (app->m_first_mouse) {
        app->m_last_x      = static_cast<float>(xpos);
        app->m_last_y      = static_cast<float>(ypos);
        app->m_first_mouse = false;
    }

    float xoffset = static_cast<float>(xpos) - app->m_last_x;
    float yoffset = app->m_last_y - static_cast<float>(ypos);
    app->m_last_x = static_cast<float>(xpos);
    app->m_last_y = static_cast<float>(ypos);

    if (app->m_cursor_captured) {
        app->m_camera->processMouseMovement(xoffset, yoffset);

        app->m_camera_moving = true;
    }
}

void Renderer::scrollCallback(GLFWwindow* m_window, double xoffset, double yoffset) {
    Renderer* app = static_cast<Renderer*>(glfwGetWindowUserPointer(m_window));
    app->m_camera->processMouseScroll(static_cast<float>(yoffset));
}
