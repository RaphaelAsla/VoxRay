#pragma once

#include <glad/glad.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class Shader {
  public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    Shader(const std::string& computePath);
    ~Shader();

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadComputeFromFile(const std::string& computePath);

    void use() const;
    void dispatch(unsigned int num_groups_x, unsigned int num_groups_y = 1, unsigned int num_groups_z = 1) const;

    void reload();

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setIVec3(const std::string& name, int x, int y, int z) const;
    void setMat4(const std::string& name, const float* value) const;
    void setBool(const std::string& name, bool value) const;

    GLuint getID() const {
        return m_program_id;
    }

    bool isValid() const {
        return m_program_id != 0;
    }

  private:
    GLuint m_program_id = 0;

    std::string m_vertex_path;
    std::string m_fragment_path;
    std::string m_compute_path;

    std::filesystem::file_time_type m_vertex_last_write;
    std::filesystem::file_time_type m_fragment_last_write;
    std::filesystem::file_time_type m_compute_last_write;

    bool m_is_compute        = false;
    bool m_has_compile_error = false;

    bool readShaderFile(const std::string& path, std::string& code) const;
    bool linkProgram(const std::vector<GLuint>& shaders);
    bool checkCompileErrors(GLuint shader, const std::string& type) const;
    bool shaderFilesModified() const;

    void cleanup();
    void trackFileModification();

    GLuint compileShader(const std::string& source, GLenum type) const;
    int    getUniformLocation(const std::string& name) const;

    mutable std::unordered_map<std::string, int> m_uniform_cache;
};
