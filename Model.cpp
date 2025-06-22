#include "Model.hpp"
#include "OBJloader.hpp"
#include <stdexcept>
#include <algorithm> 
#include <cfloat>

#undef min
#undef max

Model::Model(const std::filesystem::path& filename, ShaderProgram shader) {
    this->shader = shader;
    this->name = filename.stem().string();
    local_model_matrix = glm::mat4(1.0f);

    if (filename.empty() || filename.string().empty()) {
        return;
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    bool res = loadOBJ(filename.string(), vertices, uvs, normals);
    if (!res) {
        throw std::runtime_error("Failed to load OBJ file: " + filename.string());
    }

    std::vector<vertex> mesh_vertices;
    for (size_t i = 0; i < vertices.size(); i++) {
        vertex v;
        v.position = vertices[i];
        v.texCoord = i < uvs.size() ? uvs[i] : glm::vec2(0.0f);
        v.normal = i < normals.size() ? normals[i] : glm::vec3(0.0f, 0.0f, 1.0f);
        mesh_vertices.push_back(v);
    }

    std::vector<GLuint> indices;
    for (GLuint i = 0; i < mesh_vertices.size(); i++) {
        indices.push_back(i);
    }

    Mesh mesh(GL_TRIANGLES, shader, mesh_vertices, indices, glm::vec3(0.0f), glm::vec3(0.0f));
    meshes.push_back(mesh);
}

void Model::update(const float delta_t) {
    currentTime += delta_t; // Update current time
    float amplitude = 5.0f; // Adjust amplitude for movement range
    float speed = 1.0f;    // Adjust speed of movement

    if (name == "cube") {
        // Up-down movement (sine wave along Y-axis)
        orientation.y += 1.0f * delta_t; // Keep rotation
        origin.y = amplitude * sin(speed * currentTime);
    }
    else if (name == "sphere_tri_vnt") {
        // Left-right and back movement (cosine wave along X-axis, sine along Z)
        orientation.y += 1.0f * delta_t; // Keep rotation
        origin.x = amplitude * cos(speed * currentTime);
        origin.z = amplitude * sin(speed * currentTime);
    }
}

glm::mat4 Model::getModelMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), origin);
    glm::mat4 rx = glm::rotate(glm::mat4(1.0f), orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 ry = glm::rotate(glm::mat4(1.0f), orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rz = glm::rotate(glm::mat4(1.0f), orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

    return local_model_matrix * s * rz * ry * rx * t;
}

void Model::draw(glm::vec3 const& offset, glm::vec3 const& rotation, glm::vec3 const& scale_change) {
    shader.activate();
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

glm::vec3 Model::getMinBounds() const {
    glm::vec3 minBounds(FLT_MAX);
    glm::mat4 modelMatrix = getModelMatrix();
    for (const auto& mesh : meshes) {
        for (const auto& v : mesh.vertices) {
            glm::vec4 transformed = modelMatrix * glm::vec4(v.position, 1.0f);
            minBounds.x = std::min(minBounds.x, transformed.x);
            minBounds.y = std::min(minBounds.y, transformed.y);
            minBounds.z = std::min(minBounds.z, transformed.z);
        }
    }
    return minBounds;
}

glm::vec3 Model::getMaxBounds() const {
    glm::vec3 maxBounds(-FLT_MAX);
    glm::mat4 modelMatrix = getModelMatrix();
    for (const auto& mesh : meshes) {
        for (const auto& v : mesh.vertices) {
            glm::vec4 transformed = modelMatrix * glm::vec4(v.position, 1.0f);
            maxBounds.x = std::max(maxBounds.x, transformed.x);
            maxBounds.y = std::max(maxBounds.y, transformed.y);
            maxBounds.z = std::max(maxBounds.z, transformed.z);
        }
    }
    return maxBounds;
}