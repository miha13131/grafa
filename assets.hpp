#pragma once
#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//vertex description
struct vertex {
    glm::vec3 position;  // Pozice vrcholu
    glm::vec3 normal;    // Normala vrcholu
    glm::vec2 texCoord;  // Texturovaci koordinaty

    vertex() : position(0.0f), texCoord(0.0f), normal(0.0f) {}

    vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& norm)
        : position(pos), texCoord(tex), normal(norm) {
    }
};