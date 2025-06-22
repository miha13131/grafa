#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "OBJloader.hpp"

class Model {
public:

    std::vector<Mesh> meshes;
    std::string name;

    // Transform properties
    glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
    glm::vec3 orientation{ 0.0f, 0.0f, 0.0f }; // in radians
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
    glm::mat4 local_model_matrix{ 1.0f }; // identity (no transformation)

    // Transparency flag - ADDED FOR TASK 1
    bool transparent{ false };
    float currentTime{ 0.0f };
    ShaderProgram shader;

    // Constructor
    Model() : shader(), name(""), origin(0.0f), scale(1.0f), orientation(0.0f), local_model_matrix(1.0f), meshes() {}
    Model(const std::filesystem::path& filename, ShaderProgram shader);

    // Methods
    void update(const float delta_t);
    glm::mat4 getModelMatrix() const;
    void draw(glm::vec3 const& offset = glm::vec3(0.0f),
        glm::vec3 const& rotation = glm::vec3(0.0f),
        glm::vec3 const& scale_change = glm::vec3(1.0f));
    void draw(glm::mat4 const& model_matrix);
    glm::vec3 getMinBounds() const;
    glm::vec3 getMaxBounds() const;
};