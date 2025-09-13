#define TINYBVH_IMPLEMENTATION
#include "Scene.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>

#include "Shader.h"

const glm::vec3 Scene::face_directions[6] = {glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                             glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, 0.0f, -1.0f)};

Scene::Scene() : m_grid_width(0), m_grid_height(0), m_grid_depth(0) {}

Scene::~Scene() {
    if (m_voxel_ssbo != 0) {
        glDeleteBuffers(1, &m_voxel_ssbo);
    }
    if (m_color_ssbo != 0) {
        glDeleteBuffers(1, &m_color_ssbo);
    }
    if (m_quad_instance_vbo != 0) {
        glDeleteBuffers(1, &m_quad_instance_vbo);
    }
    if (m_bvh_ssbo != 0) {
        glDeleteBuffers(1, &m_bvh_ssbo);
    }
    if (m_wireframe_vao != 0) {
        glDeleteVertexArrays(1, &m_wireframe_vao);
    }
    if (m_wireframe_vbo != 0) {
        glDeleteBuffers(1, &m_wireframe_vbo);
    }
    if (m_wireframe_ebo != 0) {
        glDeleteBuffers(1, &m_wireframe_ebo);
    }
}

void Scene::initialize(int m_grid_width, int m_grid_height, int m_grid_depth) {
    this->m_grid_width  = m_grid_width;
    this->m_grid_height = m_grid_height;
    this->m_grid_depth  = m_grid_depth;

    m_active_voxels.clear();
    m_grid_to_compact_index.clear();

    // Random scene
    // generatePlane(glm::vec3(200, 0, 200), glm::vec2(400, 400), glm::vec3(0, 1, 0), glm::vec3(0.4f, 0.3f, 0.2f), 0.8f, 0.0f);
    // generatePlane(glm::vec3(390, 100, 200), glm::vec2(400, 200), glm::vec3(-1, 0, 0), glm::vec3(0.9f, 0.9f, 0.9f), 0.05f, 0.0f);
    // generateRandomSpheres(50, glm::vec3(0, 0, 0), glm::vec3(400, 100, 400), 0);
    // generateRandomSpheres(25, glm::vec3(0, 0, 0), glm::vec3(400, 100, 400), 1);
    // generateRandomSpheres(25, glm::vec3(0, 100, 0), glm::vec3(400, 200, 400), 2);

    // Cornell Box
    // Floor
    generatePlane(glm::vec3(160, 40, 160), glm::vec2(240, 240), glm::vec3(0, 1, 0), glm::vec3(0.725f, 0.71f, 0.68f), 0.8f, 0.0f);
    // Ceiling
    generatePlane(glm::vec3(160, 280, 160), glm::vec2(240, 240), glm::vec3(0, -1, 0), glm::vec3(0.725f, 0.71f, 0.68f), 0.8f, 0.0f);
    // Back wall
    generatePlane(glm::vec3(160, 160, 280), glm::vec2(240, 240), glm::vec3(0, 0, -1), glm::vec3(0.725f, 0.71f, 0.68f), 0.8f, 0.0f);
    // Left wall
    generatePlane(glm::vec3(40, 160, 160), glm::vec2(240, 240), glm::vec3(1, 0, 0), glm::vec3(0.63f, 0.065f, 0.05f), 0.8f, 0.0f);
    // Right wall
    generatePlane(glm::vec3(280, 160, 160), glm::vec2(240, 240), glm::vec3(-1, 0, 0), glm::vec3(0.14f, 0.45f, 0.091f), 0.8f, 0.0f);
    // Light on ceiling
    generatePlane(glm::vec3(160, 280, 160), glm::vec2(77, 77), glm::vec3(0, -1, 0), glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 30.0f);
    // 2 diffuse spheres
    generateSphere(glm::vec3(120, 100, 120), 25.0f, glm::vec3(0.9f, 0.7f, 0.3f), 0.8f, 0.0f);
    generateSphere(glm::vec3(200, 80, 200), 20.0f, glm::vec3(0.3f, 0.7f, 0.9f), 0.9f, 0.0f);
    // 2 mirror spheres
    generateSphere(glm::vec3(160, 120, 220), 30.0f, glm::vec3(0.9f, 0.9f, 0.9f), 0.05f, 0.0f);
    generateSphere(glm::vec3(100, 150, 180), 22.0f, glm::vec3(0.95f, 0.95f, 0.95f), 0.03f, 0.0f);

    createVoxelFaces();
    setupBuffers();
    updateComputeBuffer();
    buildBVH();
    setupBVHBuffers();
    setupWireframeGeometry();
    setupInstancedRendering();

    std::cout << "Scene initialized with " << m_active_voxels.size() << " active voxels" << std::endl;
}

void Scene::setupBuffers() {
    glGenBuffers(1, &m_voxel_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxel_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_active_voxels.size() * sizeof(VoxelData), m_active_voxels.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_voxel_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &m_color_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_color_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_active_voxels.size() * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_color_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Scene::updateComputeBuffer() {
    if (m_voxel_ssbo != 0 && !m_active_voxels.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_voxel_ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_active_voxels.size() * sizeof(VoxelData), m_active_voxels.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

void Scene::clearAccumulation() {
    if (m_color_ssbo != 0 && !m_active_voxels.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_color_ssbo);

        std::vector<glm::vec4> clear_colors(m_active_voxels.size(), glm::vec4(0.0f));
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, clear_colors.size() * sizeof(glm::vec4), clear_colors.data());

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        std::cout << "Cleared accumulation buffer for " << m_active_voxels.size() << " voxels" << std::endl;
    }
}

void Scene::generatePlane(const glm::vec3& center, const glm::vec2& m_size, const glm::vec3& normal, const glm::vec3& color, float roughness, float emission) {
    glm::vec3 normalized_normal = glm::normalize(normal);
    int       width             = static_cast<int>(m_size.x);
    int       depth             = static_cast<int>(m_size.y);

    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::abs(glm::dot(normalized_normal, m_up)) > 0.9f) {
        m_up = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 m_right = glm::normalize(glm::cross(m_up, normalized_normal));
    glm::vec3 forward = glm::normalize(glm::cross(normalized_normal, m_right));

    for (int u = -width / 2; u < width / 2; u++) {
        for (int v = -depth / 2; v < depth / 2; v++) {
            glm::vec3  world_pos = center + static_cast<float>(u) * m_right + static_cast<float>(v) * forward;
            glm::ivec3 grid_pos  = glm::ivec3(glm::round(world_pos));

            if (isValidPosition(grid_pos)) {
                VoxelData voxel;
                voxel.m_normal_roughness = glm::vec4(normalized_normal, roughness);
                voxel.m_albedo_emission  = glm::vec4(color, emission);
                voxel.m_grid_position    = glm::ivec4(grid_pos, 0);

                setVoxelActive(grid_pos, voxel);
            }
        }
    }
}

void Scene::generateSphere(const glm::vec3& center, float radius, const glm::vec3& color, float roughness, float emission) {
    for (int x = 0; x < m_grid_width; x++) {
        for (int y = 0; y < m_grid_height; y++) {
            for (int z = 0; z < m_grid_depth; z++) {
                glm::vec3 pos  = glm::vec3(x, y, z);
                float     dist = glm::length(pos - center);
                if (dist <= radius && dist > radius - 1.0f) {
                    VoxelData voxel;
                    glm::vec3 normal         = glm::normalize(pos - center);
                    voxel.m_normal_roughness = glm::vec4(normal, roughness);
                    voxel.m_albedo_emission  = glm::vec4(color, emission);

                    setVoxelActive(glm::ivec3(x, y, z), voxel);
                }
            }
        }
    }
}

void Scene::rebuildOptimizations() {
    createVoxelFaces();
    updateQuadInstances();
    setupBuffers();
    updateComputeBuffer();
    buildBVH();
    setupBVHBuffers();
    setupWireframeGeometry();
    setupInstancedRendering();
}

void Scene::generateRandomSpheres(int count, const glm::vec3& boundsMin, const glm::vec3& boundsMax, int materialId) {
    std::random_device rd;
    std::mt19937       gen(rd());

    std::uniform_real_distribution<float> x_dist(boundsMin.x, boundsMax.x);
    std::uniform_real_distribution<float> y_dist(boundsMin.y, boundsMax.y);
    std::uniform_real_distribution<float> z_dist(boundsMin.z, boundsMax.z);

    std::uniform_real_distribution<float> radius_dist(5.0f, 20.0f);
    std::uniform_real_distribution<float> color_dist(0.1f, 1.0f);

    for (int i = 0; i < count; i++) {
        glm::vec3 m_position(x_dist(gen), y_dist(gen), z_dist(gen));
        float     radius = radius_dist(gen);
        glm::vec3 color(color_dist(gen), color_dist(gen), color_dist(gen));

        float roughness = 0.5f;
        float emission  = 0.0f;

        switch (materialId) {
            case 0:
                roughness = std::uniform_real_distribution<float>(0.3f, 0.8f)(gen);
                emission  = 0.0f;
                break;
            case 1:
                roughness = std::uniform_real_distribution<float>(0.0f, 0.1f)(gen);
                emission  = 0.0f;
                break;
            case 2:
                roughness = 0.0f;
                emission  = std::uniform_real_distribution<float>(20.0f, 50.0f)(gen);
                color =
                    glm::vec3(std::uniform_real_distribution<float>(0.7f, 1.0f)(gen), std::uniform_real_distribution<float>(0.7f, 1.0f)(gen), std::uniform_real_distribution<float>(0.5f, 1.0f)(gen));
                break;
        }

        generateSphere(m_position, radius, color, roughness, emission);
    }
}

int Scene::getVoxelIndex(const glm::ivec3& pos) const {
    if (!isValidPosition(pos)) {
        return -1;
    }
    return pos.x + pos.y * m_grid_width + pos.z * m_grid_width * m_grid_height;
}

bool Scene::isValidPosition(const glm::ivec3& pos) const {
    return pos.x >= 0 && pos.x < m_grid_width && pos.y >= 0 && pos.y < m_grid_height && pos.z >= 0 && pos.z < m_grid_depth;
}

void Scene::setupInstancedRendering() {
    m_quad_geometry = std::make_unique<Voxel>(glm::vec3(0.0f), 1.0f);
    m_quad_geometry->generateQuadGeometry();

    updateQuadInstances();

    glGenBuffers(1, &m_quad_instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_quad_instances.size() * sizeof(QuadInstance), m_quad_instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Scene::createVoxelFaces() {
    std::vector<VoxelData>       new_active_voxels;
    std::unordered_map<int, int> new_grid_to_compact_index;

    for (int i = 0; i < m_active_voxels.size(); i++) {
        glm::ivec3 pos = glm::ivec3(m_active_voxels[i].m_grid_position);

        if (hasVisibleFace(pos)) {
            int grid_index                        = getVoxelIndex(pos);
            new_grid_to_compact_index[grid_index] = new_active_voxels.size();
            new_active_voxels.push_back(m_active_voxels[i]);
        }
    }

    m_active_voxels         = std::move(new_active_voxels);
    m_grid_to_compact_index = std::move(new_grid_to_compact_index);

    m_quad_instances.clear();

    for (int i = 0; i < m_active_voxels.size(); i++) {
        glm::ivec3 pos = glm::ivec3(m_active_voxels[i].m_grid_position);

        for (int dir = 0; dir < 6; dir++) {
            glm::ivec3 neighbor_pos = pos + glm::ivec3(Scene::face_directions[dir]);
            if (!hasNeighbor(neighbor_pos)) {
                QuadInstance quad;

                quad.m_position    = glm::vec3(pos) + Scene::face_directions[dir] * 0.5f;
                quad.m_normal      = Scene::face_directions[dir];
                quad.m_voxel_index = i;
                m_quad_instances.push_back(quad);
            }
        }
    }

    std::cout << "Culled internal voxels and generated " << m_quad_instances.size() << " quad instances from " << m_active_voxels.size() << " visible voxels" << std::endl;
}

bool Scene::hasNeighbor(const glm::ivec3& pos) const {
    if (!isValidPosition(pos))
        return false;
    int grid_index = getVoxelIndex(pos);
    return m_grid_to_compact_index.find(grid_index) != m_grid_to_compact_index.end();
}

bool Scene::hasVisibleFace(const glm::ivec3& pos) const {
    for (int dir = 0; dir < 6; dir++) {
        glm::ivec3 neighbor_pos = pos + glm::ivec3(Scene::face_directions[dir]);
        if (!hasNeighbor(neighbor_pos)) {
            return true;
        }
    }
    return false;
}

void Scene::updateQuadInstances() {
    if (m_quad_instance_vbo != 0 && !m_quad_instances.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_quad_instance_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_quad_instances.size() * sizeof(QuadInstance), m_quad_instances.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Scene::render(Shader* shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!shader || !m_quad_geometry || m_quad_instances.empty())
        return;

    GLuint vao = m_quad_geometry->getVAO();
    if (vao == 0)
        return;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_instance_vbo);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offsetof(QuadInstance, m_position));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(QuadInstance), (void*)offsetof(QuadInstance, m_normal));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribIPointer(5, 1, GL_INT, sizeof(QuadInstance), (void*)offsetof(QuadInstance, m_voxel_index));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    const auto& indices = m_quad_geometry->getIndices();
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(m_quad_instances.size()));

    glBindVertexArray(0);
}

void Scene::setVoxelActive(const glm::ivec3& pos, const VoxelData& data) {
    int grid_index = getVoxelIndex(pos);
    if (grid_index < 0 || !isValidPosition(pos))
        return;

    VoxelData data_with_pos       = data;
    data_with_pos.m_grid_position = glm::ivec4(pos.x, pos.y, pos.z, 0);

    auto it = m_grid_to_compact_index.find(grid_index);
    if (it == m_grid_to_compact_index.end()) {
        m_grid_to_compact_index[grid_index] = m_active_voxels.size();
        m_active_voxels.push_back(data_with_pos);
    } else {
        m_active_voxels[it->second] = data_with_pos;
    }
}

static Scene* g_scene = nullptr;

static void getVoxelAABB(const unsigned chunkIndex, tinybvh::bvhvec3& bmin, tinybvh::bvhvec3& bmax) {
    if (!g_scene)
        return;
    g_scene->getVoxelAABB(chunkIndex, bmin, bmax);
}

void Scene::buildBVH() {
    if (!m_active_voxels.empty()) {
        std::cout << "Building BVH from " << m_active_voxels.size() << " active voxels..." << std::endl;

        g_scene = this;

        m_bvh.Build(::getVoxelAABB, static_cast<uint32_t>(m_active_voxels.size()));

        g_scene = nullptr;

        for (uint32_t i = 0; i < m_bvh.usedNodes; i++) {
            auto& node = m_bvh.bvhNode[i];
            if (node.isLeaf()) {
                glm::vec3  voxel_center = glm::vec3((node.aabbMin.x + node.aabbMax.x) * 0.5f, (node.aabbMin.y + node.aabbMax.y) * 0.5f, (node.aabbMin.z + node.aabbMax.z) * 0.5f);
                glm::ivec3 voxel_pos    = glm::ivec3(floor(voxel_center.x + 0.5f), floor(voxel_center.y + 0.5f), floor(voxel_center.z + 0.5f));

                int  grid_index = getVoxelIndex(voxel_pos);
                auto it         = m_grid_to_compact_index.find(grid_index);
                if (it != m_grid_to_compact_index.end()) {
                    node.leftFirst = static_cast<uint32_t>(it->second);
                }
            }
        }

        std::cout << "BVH built with " << m_bvh.usedNodes << " nodes for " << m_active_voxels.size() << " voxels" << std::endl;
        std::cout << "Leaf nodes updated with direct voxel array indices" << std::endl;

        generateBVHWireframeGeometry();
        setupWireframeGeometry();
    }
}

void Scene::setupBVHBuffers() {
    if (m_bvh.usedNodes == 0 || m_active_voxels.empty())
        return;

    if (m_bvh_ssbo != 0) {
        glDeleteBuffers(1, &m_bvh_ssbo);
        m_bvh_ssbo = 0;
    }

    glGenBuffers(1, &m_bvh_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_bvh_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, m_bvh.usedNodes * sizeof(tinybvh::BVH::BVHNode), m_bvh.bvhNode, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_bvh_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::cout << "BVH buffers created: " << m_bvh.usedNodes << " nodes for " << m_active_voxels.size() << " voxels" << std::endl;
    std::cout << "BVH node 0: min(" << m_bvh.bvhNode[0].aabbMin.x << "," << m_bvh.bvhNode[0].aabbMin.y << "," << m_bvh.bvhNode[0].aabbMin.z << ") max(" << m_bvh.bvhNode[0].aabbMax.x << ","
              << m_bvh.bvhNode[0].aabbMax.y << "," << m_bvh.bvhNode[0].aabbMax.z << ")" << std::endl;
}

void Scene::setupWireframeGeometry() {
    if (m_wireframe_vertices.empty() || m_wireframe_indices.empty()) {
        return;
    }

    if (m_wireframe_vao == 0) {
        glGenVertexArrays(1, &m_wireframe_vao);
        glGenBuffers(1, &m_wireframe_vbo);
        glGenBuffers(1, &m_wireframe_ebo);
    }

    glBindVertexArray(m_wireframe_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_wireframe_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_wireframe_vertices.size() * sizeof(glm::vec3), m_wireframe_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wireframe_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_wireframe_indices.size() * sizeof(unsigned int), m_wireframe_indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Scene::renderBVHWireframe(Shader* m_wireframe_shader, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_wireframe_shader || m_wireframe_vao == 0 || m_wireframe_indices.empty())
        return;

    m_wireframe_shader->use();
    m_wireframe_shader->setMat4("uView", glm::value_ptr(view));
    m_wireframe_shader->setMat4("uProjection", glm::value_ptr(projection));
    m_wireframe_shader->setVec3("uColor", 0.0f, 1.0f, 0.0f);
    m_wireframe_shader->setFloat("uOpacity", 0.8f);

    glBindVertexArray(m_wireframe_vao);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glDrawElements(GL_LINES, static_cast<GLsizei>(m_wireframe_indices.size()), GL_UNSIGNED_INT, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void Scene::generateBVHWireframeGeometry() {
    if (m_bvh.usedNodes == 0) {
        return;
    }

    m_wireframe_vertices.clear();
    m_wireframe_indices.clear();

    std::function<void(int)> traverse_node = [&](int nodeIdx) {
        if (nodeIdx >= m_bvh.usedNodes) {
            return;
        }

        auto& node = m_bvh.bvhNode[nodeIdx];

        glm::vec3 min_pos = glm::vec3(node.aabbMin.x, node.aabbMin.y, node.aabbMin.z);
        glm::vec3 max_pos = glm::vec3(node.aabbMax.x, node.aabbMax.y, node.aabbMax.z);

        unsigned int base_index = m_wireframe_vertices.size();

        m_wireframe_vertices.push_back(glm::vec3(min_pos.x, min_pos.y, min_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(max_pos.x, min_pos.y, min_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(max_pos.x, max_pos.y, min_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(min_pos.x, max_pos.y, min_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(min_pos.x, min_pos.y, max_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(max_pos.x, min_pos.y, max_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(max_pos.x, max_pos.y, max_pos.z));
        m_wireframe_vertices.push_back(glm::vec3(min_pos.x, max_pos.y, max_pos.z));

        unsigned int box_indices[] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

        for (int i = 0; i < 24; i++) {
            m_wireframe_indices.push_back(base_index + box_indices[i]);
        }

        bool is_leaf = (node.triCount > 0);
        if (!is_leaf) {
            traverse_node(node.leftFirst);
            traverse_node(node.leftFirst + 1);
        }
    };

    traverse_node(0);

    std::cout << "Generated wireframe geometry: " << m_wireframe_vertices.size() << " vertices, " << m_wireframe_indices.size() << " indices for " << m_bvh.usedNodes << " BVH nodes" << std::endl;
}
