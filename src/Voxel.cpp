#include "Voxel.h"

#include <glm/glm.hpp>

Voxel::Voxel(const glm::vec3& m_position, float m_size) : m_position(m_position), m_size(m_size) {}

Voxel::~Voxel() {
    cleanup();
}

void Voxel::generateQuadGeometry() {
    m_vertices.clear();
    m_indices.clear();

    float half_size = m_size * 0.5f;

    m_vertices.push_back({{-half_size, -half_size, 0.0f}});
    m_vertices.push_back({{half_size, -half_size, 0.0f}});
    m_vertices.push_back({{half_size, half_size, 0.0f}});
    m_vertices.push_back({{-half_size, half_size, 0.0f}});

    m_indices = {0, 1, 2, 2, 3, 0};

    setupBuffers();
}

void Voxel::setupBuffers() {
    cleanup();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_position));
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Voxel::cleanup() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
}
