#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Mesh.hpp"
#include <iostream>

Mesh::Mesh(GLenum primitive_type, ShaderProgram shader, std::vector<vertex> const& vertices,
    std::vector<GLuint> const& indices, glm::vec3 const& origin,
    glm::vec3 const& orientation, GLuint texture_id)
    : primitive_type(primitive_type),
    shader(shader),
    vertices(vertices),
    indices(indices),
    origin(origin),
    orientation(orientation),
    texture_id(texture_id),
    diffuse_material(1.0f, 1.0f, 1.0f, 1.0f) { // Výchozí bílá barva s plnou opacitou
    // Create VAO
    glCreateVertexArrays(1, &VAO);

    // Create VBO and upload vertex data
    glCreateBuffers(1, &VBO);
    glNamedBufferData(VBO, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

    // Create EBO and upload index data
    glCreateBuffers(1, &EBO);
    glNamedBufferData(EBO, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Set up attributes
    // Vertex position
    GLint position_attrib_location = glGetAttribLocation(shader.getID(), "aPos");
    if (position_attrib_location >= 0) {
        glEnableVertexArrayAttrib(VAO, position_attrib_location);
        glVertexArrayAttribFormat(VAO, position_attrib_location, 3, GL_FLOAT, GL_FALSE,
            offsetof(vertex, position));
        glVertexArrayAttribBinding(VAO, position_attrib_location, 0);
    }

    // Vertex normal
    GLint normal_attrib_location = glGetAttribLocation(shader.getID(), "aNorm");
    if (normal_attrib_location >= 0) {
        glEnableVertexArrayAttrib(VAO, normal_attrib_location);
        glVertexArrayAttribFormat(VAO, normal_attrib_location, 3, GL_FLOAT, GL_FALSE,
            offsetof(vertex, normal));
        glVertexArrayAttribBinding(VAO, normal_attrib_location, 0);
    }

    // Texture coordinates
    GLint tex_attrib_location = glGetAttribLocation(shader.getID(), "aTex");
    if (tex_attrib_location >= 0) {
        glEnableVertexArrayAttrib(VAO, tex_attrib_location);
        glVertexArrayAttribFormat(VAO, tex_attrib_location, 2, GL_FLOAT, GL_FALSE,
            offsetof(vertex, texCoord));
        glVertexArrayAttribBinding(VAO, tex_attrib_location, 0);
    }

    // Link VAO with VBO and EBO
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(vertex));
    glVertexArrayElementBuffer(VAO, EBO);
}

void Mesh::draw(glm::vec3 const& offset, glm::vec3 const& rotation) const {
    if (VAO == 0) {
        std::cerr << "VAO not initialized!\n";
        return;
    }

    // Activate texture if it exists
    if (texture_id != 0) {
        glBindTextureUnit(0, texture_id);
        GLint tex_loc = glGetUniformLocation(shader.getID(), "tex0");
        if (tex_loc >= 0) {
            glUniform1i(tex_loc, 0);
        }
    }

    // Set diffuse_material in shader
    GLint diffuse_color_loc = glGetUniformLocation(shader.getID(), "u_diffuse_color");
    if (diffuse_color_loc >= 0) {
        glUniform4fv(diffuse_color_loc, 1, glm::value_ptr(diffuse_material));
    }

    // Draw the mesh
    glBindVertexArray(VAO);
    glDrawElements(primitive_type, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void Mesh::clear() {
    if (texture_id != 0) {
        glDeleteTextures(1, &texture_id);
        texture_id = 0;
    }

    primitive_type = GL_POINT;
    vertices.clear();
    indices.clear();
    origin = glm::vec3(0.0f);
    orientation = glm::vec3(0.0f);

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}