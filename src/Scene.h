#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Voxel.h"
#include "tiny_bvh.h"

struct QuadInstance {
    glm::vec3 m_position;
    glm::vec3 m_normal;
    int       m_voxel_index;
};

class Scene {
  public:
    Scene();
    ~Scene();

    void initialize(int gridWidth = 32, int gridHeight = 16, int gridDepth = 32);
    void setupBuffers();
    void updateComputeBuffer();
    void clearAccumulation();
    void render(class Shader* shader, const glm::mat4& view, const glm::mat4& projection);
    void renderBVHWireframe(class Shader* wireframeShader, const glm::mat4& view, const glm::mat4& projection);
    void rebuildOptimizations();
    void generateRandomSpheres(int count, const glm::vec3& boundsMin, const glm::vec3& boundsMax, int materialId);

    const std::vector<VoxelData>& getActiveVoxels() const {
        return m_active_voxels;
    }

    int getGridWidth() const {
        return m_grid_width;
    }

    int getGridHeight() const {
        return m_grid_height;
    }

    int getGridDepth() const {
        return m_grid_depth;
    }

    GLuint getVoxelBuffer() const {
        return m_voxel_ssbo;
    }

    GLuint getColorBuffer() const {
        return m_color_ssbo;
    }

    int getActiveVoxelCount() const {
        return m_active_voxels.size();
    }

    int getFaceCount() const {
        return m_quad_instances.size();
    }

    GLuint getBVHBuffer() const {
        return m_bvh_ssbo;
    }

    int getBVHNodeCount() const {
        return m_bvh.usedNodes;
    }

    // Custom callback used to build the BVH
    void getVoxelAABB(const unsigned int voxelIndex, tinybvh::bvhvec3& minOut, tinybvh::bvhvec3& maxOut) const {
        if (voxelIndex >= m_active_voxels.size()) {
            return;
        }

        glm::vec3 pos = glm::vec3(m_active_voxels[voxelIndex].m_grid_position);
        minOut        = tinybvh::bvhvec3(pos.x, pos.y, pos.z) - tinybvh::bvhvec3(0.5f);
        maxOut        = tinybvh::bvhvec3(pos.x, pos.y, pos.z) + tinybvh::bvhvec3(0.5f);
    }

  private:
    int                          m_grid_width, m_grid_height, m_grid_depth;
    std::vector<VoxelData>       m_active_voxels;
    GLuint                       m_voxel_ssbo = 0;
    GLuint                       m_color_ssbo = 0;
    std::unordered_map<int, int> m_grid_to_compact_index;

    tinybvh::BVH m_bvh;
    GLuint       m_bvh_ssbo = 0;

    std::unique_ptr<Voxel>    m_quad_geometry;
    std::vector<QuadInstance> m_quad_instances;
    GLuint                    m_quad_instance_vbo = 0;

    std::vector<glm::vec3>    m_wireframe_vertices;
    std::vector<unsigned int> m_wireframe_indices;
    GLuint                    m_wireframe_vao = 0;
    GLuint                    m_wireframe_vbo = 0;
    GLuint                    m_wireframe_ebo = 0;

    void generatePlane(const glm::vec3& center, const glm::vec2& size, const glm::vec3& normal, const glm::vec3& color, float roughness, float emission);
    void generateSphere(const glm::vec3& center, float radius, const glm::vec3& color, float roughness, float emission);
    void buildBVH();
    void setupBVHBuffers();
    void setupWireframeGeometry();
    void generateBVHWireframeGeometry();
    void setupInstancedRendering();
    void createVoxelFaces();
    void updateQuadInstances();
    void setVoxelActive(const glm::ivec3& pos, const VoxelData& data);

    bool isValidPosition(const glm::ivec3& pos) const;
    bool hasNeighbor(const glm::ivec3& pos) const;
    bool hasVisibleFace(const glm::ivec3& pos) const;

    int getVoxelIndex(const glm::ivec3& pos) const;

    static const glm::vec3 face_directions[6];
};
