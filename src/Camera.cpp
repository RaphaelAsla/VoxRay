#include "Camera.h"

#include <algorithm>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

Camera::Camera(glm::vec3 m_position, glm::vec3 m_up, float m_yaw, float m_pitch)
    : m_position(m_position)
    , m_world_up(m_up)
    , m_yaw(m_yaw)
    , m_pitch(m_pitch)
    , m_movement_speed(50.0f)
    , m_mouse_sensitivity(0.1f)
    , m_fov(45.0f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float near, float far) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, near, far);
}

void Camera::processKeyboard(int direction, float deltaTime) {
    float velocity = m_movement_speed * deltaTime;

    switch (direction) {
        case FORWARD:
            m_position += m_front * velocity;
            break;
        case BACKWARD:
            m_position -= m_front * velocity;
            break;
        case LEFT:
            m_position -= m_right * velocity;
            break;
        case RIGHT:
            m_position += m_right * velocity;
            break;
        case UP:
            m_position += m_up * velocity;
            break;
        case DOWN:
            m_position -= m_up * velocity;
            break;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= m_mouse_sensitivity;
    yoffset *= m_mouse_sensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    m_fov -= yoffset;
    m_fov = std::clamp(m_fov, 1.0f, 45.0f);
}

void Camera::updateCameraVectors() {
    glm::vec3 new_front;
    new_front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    new_front.y = sin(glm::radians(m_pitch));
    new_front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front     = glm::normalize(new_front);

    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}
