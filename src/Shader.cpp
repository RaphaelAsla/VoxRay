#include "Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    loadFromFiles(vertexPath, fragmentPath);
}

Shader::Shader(const std::string& computePath) {
    loadComputeFromFile(computePath);
}

Shader::~Shader() {
    cleanup();
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    this->m_vertex_path   = vertexPath;
    this->m_fragment_path = fragmentPath;
    this->m_is_compute    = false;

    std::string vertex_code, fragment_code;

    if (!readShaderFile(vertexPath, vertex_code)) {
        std::cerr << "Failed to read vertex shader: " << vertexPath << std::endl;
        return false;
    }

    if (!readShaderFile(fragmentPath, fragment_code)) {
        std::cerr << "Failed to read fragment shader: " << fragmentPath << std::endl;
        return false;
    }

    std::cout << "Read vertex shader successfully, m_size: " << vertex_code.length() << " bytes" << std::endl;
    std::cout << "Read fragment shader successfully, m_size: " << fragment_code.length() << " bytes" << std::endl;

    GLuint vertex   = compileShader(vertex_code, GL_VERTEX_SHADER);
    GLuint fragment = compileShader(fragment_code, GL_FRAGMENT_SHADER);

    if (vertex == 0 || fragment == 0) {
        if (vertex != 0)
            glDeleteShader(vertex);
        if (fragment != 0)
            glDeleteShader(fragment);
        return false;
    }

    if (linkProgram({vertex, fragment})) {
        trackFileModification();
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        std::cout << "Graphics shaders loaded successfully!" << std::endl;
        return true;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return false;
}

bool Shader::loadComputeFromFile(const std::string& computePath) {
    this->m_compute_path = computePath;
    this->m_is_compute   = true;

    std::string compute_code;
    if (!readShaderFile(computePath, compute_code)) {
        std::cerr << "Failed to read compute shader: " << computePath << std::endl;
        return false;
    }

    std::cout << "Read compute shader successfully, m_size: " << compute_code.length() << " bytes" << std::endl;

    GLuint compute = compileShader(compute_code, GL_COMPUTE_SHADER);
    if (compute == 0) {
        return false;
    }

    if (linkProgram({compute})) {
        trackFileModification();
        glDeleteShader(compute);
        std::cout << "Compute shader loaded successfully!" << std::endl;
        return true;
    }

    glDeleteShader(compute);
    return false;
}

void Shader::use() const {
    if (m_program_id != 0) {
        glUseProgram(m_program_id);
    }
}

void Shader::dispatch(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z) const {
    if (m_program_id != 0 && m_is_compute) {
        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void Shader::reload() {
    if (!shaderFilesModified()) {
        return;
    }

    if (m_has_compile_error) {
        m_has_compile_error = false;
    }

    std::cout << "Reloading shaders..." << std::endl;

    GLuint old_program = m_program_id;
    m_program_id       = 0;

    bool success = false;
    if (m_is_compute) {
        success = loadComputeFromFile(m_compute_path);
    } else {
        success = loadFromFiles(m_vertex_path, m_fragment_path);
    }

    if (success) {
        if (old_program != 0) {
            glDeleteProgram(old_program);
        }
        m_uniform_cache.clear();
        m_has_compile_error = false;
    } else {
        m_program_id        = old_program;
        m_has_compile_error = true;
        std::cerr << "Failed to reload shaders, will try again on next file change" << std::endl;
    }

    trackFileModification();
}

bool Shader::readShaderFile(const std::string& path, std::string& code) const {
    std::ifstream shader_file(path);
    if (!shader_file) {
        std::cerr << "Error: Could not open shader file " << path << std::endl;
        return false;
    }

    std::stringstream shader_stream;
    shader_stream << shader_file.rdbuf();
    code = shader_stream.str();
    return true;
}

GLuint Shader::compileShader(const std::string& source, GLenum type) const {
    GLuint      shader = glCreateShader(type);
    const char* src    = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    std::string type_str;
    switch (type) {
        case GL_VERTEX_SHADER:
            type_str = "VERTEX";
            break;
        case GL_FRAGMENT_SHADER:
            type_str = "FRAGMENT";
            break;
        case GL_COMPUTE_SHADER:
            type_str = "COMPUTE";
            break;
        default:
            type_str = "UNKNOWN";
            break;
    }

    if (!checkCompileErrors(shader, type_str)) {
        glDeleteShader(shader);
        return 0;
    }

    std::cout << type_str << " shader compiled successfully" << std::endl;
    return shader;
}

bool Shader::linkProgram(const std::vector<GLuint>& shaders) {
    cleanup();
    m_program_id = glCreateProgram();

    for (GLuint shader : shaders) {
        glAttachShader(m_program_id, shader);
    }

    glLinkProgram(m_program_id);

    if (!checkCompileErrors(m_program_id, "PROGRAM")) {
        cleanup();
        return false;
    }

    std::cout << "Shader program linked successfully" << std::endl;
    return true;
}

void Shader::cleanup() {
    if (m_program_id != 0) {
        glDeleteProgram(m_program_id);
        m_program_id = 0;
    }
}

bool Shader::checkCompileErrors(GLuint shader, const std::string& type) const {
    GLint  success;
    GLchar info_log[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR (" << type << "):\n" << info_log << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, info_log);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR:\n" << info_log << std::endl;
            return false;
        }
    }

    return true;
}

void Shader::trackFileModification() {
    try {
        if (m_is_compute && !m_compute_path.empty()) {
            m_compute_last_write = std::filesystem::last_write_time(m_compute_path);
        } else {
            if (!m_vertex_path.empty()) {
                m_vertex_last_write = std::filesystem::last_write_time(m_vertex_path);
            }
            if (!m_fragment_path.empty()) {
                m_fragment_last_write = std::filesystem::last_write_time(m_fragment_path);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error tracking file modification: " << e.what() << std::endl;
    }
}

bool Shader::shaderFilesModified() const {
    try {
        bool modified = false;
        if (m_is_compute && !m_compute_path.empty()) {
            modified = std::filesystem::last_write_time(m_compute_path) != m_compute_last_write;
        } else {
            bool vertex_modified   = false;
            bool fragment_modified = false;

            if (!m_vertex_path.empty()) {
                vertex_modified = std::filesystem::last_write_time(m_vertex_path) != m_vertex_last_write;
            }
            if (!m_fragment_path.empty()) {
                fragment_modified = std::filesystem::last_write_time(m_fragment_path) != m_fragment_last_write;
            }

            modified = vertex_modified || fragment_modified;
        }

        return modified;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error checking file modification: " << e.what() << std::endl;
        return false;
    }
}

int Shader::getUniformLocation(const std::string& name) const {
    auto it = m_uniform_cache.find(name);
    if (it != m_uniform_cache.end()) {
        return it->second;
    }

    int location          = glGetUniformLocation(m_program_id, name.c_str());
    m_uniform_cache[name] = location;
    return location;
}

void Shader::setInt(const std::string& name, int value) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void Shader::setFloat(const std::string& name, float value) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setVec2(const std::string& name, float x, float y) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void Shader::setIVec3(const std::string& name, int x, int y, int z) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniform3i(location, x, y, z);
    }
}

void Shader::setMat4(const std::string& name, const float* value) const {
    int location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, value);
    }
}

void Shader::setBool(const std::string& name, bool value) const {
    setInt(name, value ? 1 : 0);
}
