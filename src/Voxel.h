#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 m_position;
};

struct VoxelData {
    glm::vec4  m_normal_roughness;
    glm::vec4  m_albedo_emission;
    glm::ivec4 m_grid_position;
};

enum VoxelMaterial : uint32_t { MATERIAL_DIFFUSE = 0, MATERIAL_MIRROR = 1, MATERIAL_EMISSIVE = 2, MATERIAL_METAL = 3 };

class Voxel {
  public:
    Voxel(const glm::vec3& position = glm::vec3(0.0f), float size = 1.0f);
    ~Voxel();

    void generateQuadGeometry();
    void setupBuffers();
    void cleanup();

    const std::vector<Vertex>& getVertices() const {
        return m_vertices;
    }

    const std::vector<unsigned int>& getIndices() const {
        return m_indices;
    }

    void setPosition(const glm::vec3& pos) {
        m_position = pos;
    }

    void setSize(float s) {
        m_size = s;
    }

    glm::vec3 getPosition() const {
        return m_position;
    }

    float getSize() const {
        return m_size;
    }

    GLuint getVAO() const {
        return m_vao;
    }

  private:
    glm::vec3 m_position;
    float     m_size;

    std::vector<Vertex>       m_vertices;
    std::vector<unsigned int> m_indices;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
};
