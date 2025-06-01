#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "ShaderProgram.hpp"
#include "assets.hpp"

class Mesh {
public:
    // Full constructor with all parameters
    Mesh(GLenum primitive_type, ShaderProgram shader, std::vector<vertex> const& vertices,
        std::vector<GLuint> const& indices, glm::vec3 const& origin,
        glm::vec3 const& orientation, GLuint texture_id = 0);

    // Methods
    void draw(glm::vec3 const& offset = glm::vec3(0.0f), glm::vec3 const& rotation = glm::vec3(0.0f)) const;
    void clear();

    // Public members (for OBJLoader to set material)
    std::vector<vertex> vertices;
    std::vector<GLuint> indices;
    glm::vec3 origin;
    glm::vec3 orientation;
    GLuint texture_id{ 0 };
    GLenum primitive_type = GL_POINT;
    ShaderProgram shader;

    // Material properties
    glm::vec4 ambient_material{ 1.0f };
    glm::vec4 diffuse_material{ 1.0f };
    glm::vec4 specular_material{ 1.0f };
    float reflectivity{ 1.0f };

private:
    // OpenGL buffer IDs
    unsigned int VAO{ 0 }, VBO{ 0 }, EBO{ 0 };
};