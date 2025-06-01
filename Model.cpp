#include "Model.hpp"
#include "OBJloader.hpp"
#include <stdexcept>

Model::Model(const std::filesystem::path& filename, ShaderProgram shader) {
    this->shader = shader;
    this->name = filename.stem().string();
    local_model_matrix = glm::mat4(1.0f); // Inicializace transformační matice

    // Pokud je název souboru prázdný, přeskoč načítání .obj souboru
    if (filename.empty() || filename.string().empty()) {
        return; // Nebo nastavte výchozí hodnoty pro meshes, pokud je potřeba
    }

    // Load OBJ file
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    bool res = loadOBJ(filename.string(), vertices, uvs, normals);
    if (!res) {
        throw std::runtime_error("Failed to load OBJ file: " + filename.string());
    }

    // Convert loaded data into our vertex structure format
    std::vector<vertex> mesh_vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertex v;
        v.position = vertices[i];
        if (i < uvs.size()) {
            v.texCoord = uvs[i];
        }
        else {
            v.texCoord = glm::vec2(0.0f);
        }
        if (i < normals.size()) {
            v.normal = normals[i];
        }
        else {
            v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        mesh_vertices.push_back(v);
    }

    // Create indices - simple sequential indexing
    std::vector<GLuint> indices;
    for (GLuint i = 0; i < mesh_vertices.size(); i++) {
        indices.push_back(i);
    }

    // Create mesh
    Mesh mesh(GL_TRIANGLES, shader, mesh_vertices, indices, glm::vec3(0.0f), glm::vec3(0.0f));
    meshes.push_back(mesh);
}

void Model::update(const float delta_t) {
    // Here you can implement automatic animations
    // Example: orientation.y += 1.0f * delta_t; // rotation around Y axis
}

glm::mat4 Model::getModelMatrix() const {
    // Compute the complete transformation matrix
    glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
    glm::mat4 rx = glm::rotate(glm::mat4(1.0f), orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 ry = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rz = glm::rotate(glm::mat4(1.0f), orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

    return local_model_matrix * s * rz * ry * rx * t;
}

void Model::draw(glm::vec3 const& offset, glm::vec3 const& rotation, glm::vec3 const& scale_change) {
    // Activate shader
    shader.activate();

    // Draw all meshes
    for (auto& mesh : meshes) {
        mesh.draw();
    }
}

void Model::draw(glm::mat4 const& model_matrix) {
    shader.activate();
    for (auto& mesh : meshes) {
        mesh.draw();
    }
}

