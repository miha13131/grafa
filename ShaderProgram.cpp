#include "ShaderProgram.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

// Helper function to read a text file
std::string ShaderProgram::textFileRead(const std::filesystem::path& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename.string());
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Compiles a shader from source
GLuint ShaderProgram::compile_shader(const std::filesystem::path& source_file, const GLenum type) {
    std::string source = textFileRead(source_file);
    const char* source_cstr = source.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::string log = getShaderInfoLog(shader);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed: " + log);
    }
    return shader;
}

// Links shaders into a program
GLuint ShaderProgram::link_shader(const std::vector<GLuint> shader_ids) {
    GLuint program = glCreateProgram();
    for (GLuint id : shader_ids) {
        glAttachShader(program, id);
    }
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        std::string log = getProgramInfoLog(program);
        glDeleteProgram(program);
        throw std::runtime_error("Shader linking failed: " + log);
    }

    // Clean up shaders (they're now linked into the program)
    for (GLuint id : shader_ids) {
        glDeleteShader(id);
    }

    return program;
}

// Constructor implementation
ShaderProgram::ShaderProgram(const std::filesystem::path& VS_file, const std::filesystem::path& FS_file) {
    GLuint vertexShader = compile_shader(VS_file, GL_VERTEX_SHADER);
    GLuint fragmentShader = compile_shader(FS_file, GL_FRAGMENT_SHADER);
    ID = link_shader({ vertexShader, fragmentShader });
}

// Error log helpers
std::string ShaderProgram::getShaderInfoLog(const GLuint obj) {
    GLint logLength;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(logLength, ' ');
    glGetShaderInfoLog(obj, logLength, nullptr, log.data());
    return log;
}

std::string ShaderProgram::getProgramInfoLog(const GLuint obj) {
    GLint logLength;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(logLength, ' ');
    glGetProgramInfoLog(obj, logLength, nullptr, log.data());
    return log;
}

// Uniform setters (example for float, others follow similarly)
void ShaderProgram::setUniform(const std::string& name, const float val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniform1f(loc, val);
}
// ... (Implement other setUniform methods similarly)
// int
void ShaderProgram::setUniform(const std::string& name, const int val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniform1i(loc, val);
}

// vec3
void ShaderProgram::setUniform(const std::string& name, const glm::vec3 val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniform3f(loc, val.x, val.y, val.z);
}

// vec4
void ShaderProgram::setUniform(const std::string& name, const glm::vec4 val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniform4f(loc, val.x, val.y, val.z, val.w);
}

// mat3
void ShaderProgram::setUniform(const std::string& name, const glm::mat3 val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}

// mat4
void ShaderProgram::setUniform(const std::string& name, const glm::mat4 val) {
    GLint loc = glGetUniformLocation(ID, name.c_str());
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}