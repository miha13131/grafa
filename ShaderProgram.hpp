#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <filesystem>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>  // Pøidáváme include pro glm
#include <glm/gtc/type_ptr.hpp>  // Pro glm::value_ptr

class ShaderProgram {
public:
    // you can add more constructors for pipeline with GS, TS etc.
    ShaderProgram(void) = default; //does nothing
    ShaderProgram(const std::filesystem::path& VS_file, const std::filesystem::path& FS_file); // TODO: implementation of load, compile, and link shader
    // V ShaderProgram.hpp
    void activate(void) const { glUseProgram(ID); };
    void deactivate(void) const { glUseProgram(0); };
    void clear(void) { 	//deallocate shader program
        deactivate();
        glDeleteProgram(ID);
        ID = 0;
    }
    // Getter pro ID
    GLuint getID() const { return ID; }
    // set uniform according to name 
    // https://docs.gl/gl4/glUniform
    void setUniform(const std::string& name, const float val);
    void setUniform(const std::string& name, const int val);
    void setUniform(const std::string& name, const glm::vec3 val);
    void setUniform(const std::string& name, const glm::vec4 val);
    void setUniform(const std::string& name, const glm::mat3 val);
    void setUniform(const std::string& name, const glm::mat4 val);
private:
    GLuint ID{ 0 }; // default = 0, empty shader
    std::string getShaderInfoLog(const GLuint obj);
    std::string getProgramInfoLog(const GLuint obj);
    GLuint compile_shader(const std::filesystem::path& source_file, const GLenum type);
    GLuint link_shader(const std::vector<GLuint> shader_ids);
    std::string textFileRead(const std::filesystem::path& filename); // load text file
};