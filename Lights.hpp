#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    DirectionalLight(const glm::vec3& dir, const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec)
        : direction(dir), ambient(amb), diffuse(diff), specular(spec) {
    }
    void apply(GLuint shaderID, int index) const;
    static DirectionalLight createDefault();
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;

    PointLight(const glm::vec3& pos, const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec,
        float c, float l, float q)
        : position(pos), ambient(amb), diffuse(diff), specular(spec), constant(c), linear(l), quadratic(q) {
    }
    void apply(GLuint shaderID, int index) const;
    static PointLight createDefault(const glm::vec3& position, const glm::vec3& color);
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;

    SpotLight(const glm::vec3& pos, const glm::vec3& dir, float cut, float outerCut,
        const glm::vec3& amb, const glm::vec3& diff, const glm::vec3& spec,
        float c, float l, float q)
        : position(pos), direction(dir), cutOff(cut), outerCutOff(outerCut),
        ambient(amb), diffuse(diff), specular(spec), constant(c), linear(l), quadratic(q) {
    }
    void apply(GLuint shaderID, int index) const;
    static SpotLight createDefault(const glm::vec3& pos, const glm::vec3& dir);
};

struct AmbientLight {
    glm::vec3 color;
    AmbientLight(const glm::vec3& col) : color(col) {}
    void apply(GLuint shaderID, int index) const;
    static AmbientLight createDefault(const glm::vec3& color);
};

class Lights {
public:
    DirectionalLight sun;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    AmbientLight ambientLight;

    Lights() : sun(DirectionalLight::createDefault()), ambientLight(AmbientLight::createDefault(glm::vec3(0.2f))) {}
    void initDirectionalLight();
    void initPointLight(const glm::vec3& position, const glm::vec3& color);
    void initSpotLight(const glm::vec3& pos, const glm::vec3& dir);
    void initAmbientLight(const glm::vec3& color);
    void apply(GLuint shaderID) const;
};

#endif