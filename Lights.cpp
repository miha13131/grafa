#include "Lights.hpp"
#include <glm/gtc/type_ptr.hpp>

DirectionalLight DirectionalLight::createDefault() {
    return DirectionalLight(
        glm::vec3(-0.5f, -1.0f, -0.2f),
        glm::vec3(0.2f),
        glm::vec3(0.5f),
        glm::vec3(1.0f)
    );
}

PointLight PointLight::createDefault(const glm::vec3& position, const glm::vec3& color) {
    return PointLight(
        position,
        color * 0.1f,
        color * 0.8f,
        glm::vec3(1.0f),
        1.0f, 0.001f, 0.00001f
    );
}

SpotLight SpotLight::createDefault(const glm::vec3& pos, const glm::vec3& dir) {
    return SpotLight(
        pos,
        dir,
        glm::cos(glm::radians(15.0f)),
        glm::cos(glm::radians(30.0f)),
        glm::vec3(0.1f),
        glm::vec3(1.0f),
        glm::vec3(1.0f),
        1.0f, 0.01f, 0.001f
    );
}

AmbientLight AmbientLight::createDefault(const glm::vec3& color) {
    return AmbientLight(color);
}

void DirectionalLight::apply(GLuint shaderID, int index) const {
    std::string prefix = "dirLights[" + std::to_string(index) + "]";
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".direction").c_str()), 1, glm::value_ptr(direction));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".ambient").c_str()), 1, glm::value_ptr(ambient));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".diffuse").c_str()), 1, glm::value_ptr(diffuse));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".specular").c_str()), 1, glm::value_ptr(specular));
}

void PointLight::apply(GLuint shaderID, int index) const {
    std::string prefix = "pointLights[" + std::to_string(index) + "]";
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".position").c_str()), 1, glm::value_ptr(position));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".ambient").c_str()), 1, glm::value_ptr(ambient));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".diffuse").c_str()), 1, glm::value_ptr(diffuse));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".specular").c_str()), 1, glm::value_ptr(specular));
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".constant").c_str()), constant);
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".linear").c_str()), linear);
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".quadratic").c_str()), quadratic);
}

void SpotLight::apply(GLuint shaderID, int index) const {
    std::string prefix = "spotLights[" + std::to_string(index) + "]";
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".position").c_str()), 1, glm::value_ptr(position));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".direction").c_str()), 1, glm::value_ptr(direction));
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".cutOff").c_str()), cutOff);
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".outerCutOff").c_str()), outerCutOff);
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".ambient").c_str()), 1, glm::value_ptr(ambient));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".diffuse").c_str()), 1, glm::value_ptr(diffuse));
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, (prefix + ".specular").c_str()), 1, glm::value_ptr(specular));
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".constant").c_str()), constant);
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".linear").c_str()), linear);
    glProgramUniform1f(shaderID, glGetUniformLocation(shaderID, (prefix + ".quadratic").c_str()), quadratic);
}

void AmbientLight::apply(GLuint shaderID, int /*index*/) const {
    glProgramUniform3fv(shaderID, glGetUniformLocation(shaderID, "ambientLight.color"), 1, glm::value_ptr(color));
}

void Lights::initDirectionalLight() {
    sun = DirectionalLight::createDefault();
}

void Lights::initPointLight(const glm::vec3& position, const glm::vec3& color) {
    pointLights.emplace_back(PointLight::createDefault(position, color));
}

void Lights::initSpotLight(const glm::vec3& pos, const glm::vec3& dir) {
    spotLights.emplace_back(SpotLight::createDefault(pos, dir));
}

void Lights::initAmbientLight(const glm::vec3& color) {
    ambientLight = AmbientLight::createDefault(color);
}

void Lights::apply(GLuint shaderID) const {
    glProgramUniform1i(shaderID, glGetUniformLocation(shaderID, "numPointLights"), static_cast<int>(pointLights.size()));
    glProgramUniform1i(shaderID, glGetUniformLocation(shaderID, "numSpotLights"), static_cast<int>(spotLights.size()));
    ambientLight.apply(shaderID, 0);
    sun.apply(shaderID, 0);
    for (size_t i = 0; i < pointLights.size(); ++i) {
        pointLights[i].apply(shaderID, i);
    }
    for (size_t i = 0; i < spotLights.size(); ++i) {
        spotLights[i].apply(shaderID, i);
    }
}