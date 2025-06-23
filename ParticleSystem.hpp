#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include <random>
#include <iostream>

class ParticleSystem {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        float life;
        bool active = false;
    };

    ParticleSystem(size_t maxParticles = 400, float emissionRate = 15.0f)
        : maxParticles(maxParticles), emissionRate(emissionRate) {
    }

    void initialize(GLuint shaderID) {
        shaderProgram = shaderID;
        if (shaderProgram == 0) {
            std::cerr << "ParticleSystem: Invalid shader program ID" << std::endl;
        }
        particles.resize(maxParticles);
        for (auto& p : particles)
            p.active = false;
    }

    void update(float dt, const glm::vec3& emitterPos, float baseY) {
        activeCount = 0;
        for (auto& p : particles) {
            if (p.active) {
                p.position += p.velocity * dt;
                p.lifetime -= dt;
                p.life = p.lifetime / particleLifetime;
                if (p.position.y < 0.0f) {
                    p.position.y = 0.0f;
                    p.velocity = glm::vec3(0.0f);
                }
                if (p.lifetime <= 0.0f) {
                    p.active = false;
                }
                else {
                    activeCount++;
                }
            }
        }

        emitAccumulator += emissionRate * dt;
        while (emitAccumulator >= 1.0f && activeCount < maxParticles) {
            emitParticle(emitterPos, baseY);
            emitAccumulator -= 1.0f;
            activeCount++;
        }
    }

    void render(const glm::mat4& projection, const glm::mat4& view) {
        if (activeCount == 0 || shaderProgram == 0)
            return;

        std::vector<glm::vec4> particleData;
        for (const auto& p : particles) {
            if (p.active) {
                particleData.emplace_back(p.position, p.life);
            }
        }

        if (particleData.empty())
            return;

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(shaderProgram);

        GLuint vao, vbo;
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);
        glNamedBufferData(vbo, particleData.size() * sizeof(glm::vec4), particleData.data(), GL_DYNAMIC_DRAW);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec4));

        glEnableVertexArrayAttrib(vao, 0); // position
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        glEnableVertexArrayAttrib(vao, 1); // life
        glVertexArrayAttribFormat(vao, 1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glVertexArrayAttribBinding(vao, 1, 0);

        glBindVertexArray(vao);

        glm::mat4 model = glm::mat4(1.0f);
        glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uM_m"), 1, GL_FALSE, &model[0][0]);
        glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uV_m"), 1, GL_FALSE, &view[0][0]);
        glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uP_m"), 1, GL_FALSE, &projection[0][0]);

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleData.size()));

        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>
#include <random>
#include <iostream>

        class ParticleSystem {
        public:
            struct Particle {
                glm::vec3 position;
                glm::vec3 velocity;
                float lifetime;
                float life;
                bool active = false;
            };

            ParticleSystem(size_t maxParticles = 400, float emissionRate = 15.0f)
                : maxParticles(maxParticles), emissionRate(emissionRate) {
            }

            void initialize(GLuint shaderID) {
                shaderProgram = shaderID;
                if (shaderProgram == 0) {
                    std::cerr << "ParticleSystem: Invalid shader program ID" << std::endl;
                }
                particles.resize(maxParticles);
                for (auto& p : particles)
                    p.active = false;
            }

            void update(float dt, const glm::vec3& emitterPos, float baseY) {
                activeCount = 0;
                for (auto& p : particles) {
                    if (p.active) {
                        p.position += p.velocity * dt;
                        p.lifetime -= dt;
                        p.life = p.lifetime / particleLifetime;
                        if (p.position.y < 0.0f) {
                            p.position.y = 0.0f;
                            p.velocity = glm::vec3(0.0f);
                        }
                        if (p.lifetime <= 0.0f) {
                            p.active = false;
                        }
                        else {
                            activeCount++;
                        }
                    }
                }

                emitAccumulator += emissionRate * dt;
                while (emitAccumulator >= 1.0f && activeCount < maxParticles) {
                    emitParticle(emitterPos, baseY);
                    emitAccumulator -= 1.0f;
                    activeCount++;
                }
            }

            void render(const glm::mat4& projection, const glm::mat4& view) {
                if (activeCount == 0 || shaderProgram == 0)
                    return;

                std::vector<glm::vec4> particleData;
                for (const auto& p : particles) {
                    if (p.active) {
                        particleData.emplace_back(p.position, p.life);
                    }
                }

                if (particleData.empty())
                    return;

                glEnable(GL_PROGRAM_POINT_SIZE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUseProgram(shaderProgram);

                GLuint vao, vbo;
                glCreateVertexArrays(1, &vao);
                glCreateBuffers(1, &vbo);
                glNamedBufferData(vbo, particleData.size() * sizeof(glm::vec4), particleData.data(), GL_DYNAMIC_DRAW);

                glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(glm::vec4));

                glEnableVertexArrayAttrib(vao, 0); // position
                glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
                glVertexArrayAttribBinding(vao, 0, 0);

                glEnableVertexArrayAttrib(vao, 1); // life
                glVertexArrayAttribFormat(vao, 1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
                glVertexArrayAttribBinding(vao, 1, 0);

                glBindVertexArray(vao);

                glm::mat4 model = glm::mat4(1.0f);
                glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uM_m"), 1, GL_FALSE, &model[0][0]);
                glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uV_m"), 1, GL_FALSE, &view[0][0]);
                glProgramUniformMatrix4fv(shaderProgram, glGetUniformLocation(shaderProgram, "uP_m"), 1, GL_FALSE, &projection[0][0]);

                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(particleData.size()));

                glBindVertexArray(0);
                glDeleteBuffers(1, &vbo);
                glDeleteVertexArrays(1, &vao);

                glDisable(GL_BLEND);
                glDisable(GL_PROGRAM_POINT_SIZE);
            }

            void cleanup() {
                // Optional: not used
            }

        private:
            std::vector<Particle> particles;
            size_t maxParticles;
            float emissionRate;
            float emitAccumulator = 0.0f;
            size_t activeCount = 0;
            GLuint shaderProgram = 0;
            const float particleLifetime = 5.0f;

            void emitParticle(const glm::vec3& origin, float baseY) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<float> spread(-0.5f, 0.5f);
                static std::uniform_real_distribution<float> velocity(-0.3f, 0.3f);

                for (auto& p : particles) {
                    if (!p.active) {
                        p.position = glm::vec3(origin.x + spread(gen), baseY, origin.z + spread(gen));
                        p.velocity = glm::vec3(velocity(gen), -1.5f, velocity(gen));
                        p.lifetime = particleLifetime;
                        p.life = 1.0f;
                        p.active = true;
                        break;
                    }
                }
            }
        };

        glDisable(GL_PROGRAM_POINT_SIZE);
    }

    void cleanup() {
        // Optional: not used
    }

private:
    std::vector<Particle> particles;
    size_t maxParticles;
    float emissionRate;
    float emitAccumulator = 0.0f;
    size_t activeCount = 0;
    GLuint shaderProgram = 0;
    const float particleLifetime = 5.0f;

    void emitParticle(const glm::vec3& origin, float baseY) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> spread(-0.5f, 0.5f);
        static std::uniform_real_distribution<float> velocity(-0.3f, 0.3f);

        for (auto& p : particles) {
            if (!p.active) {
                p.position = glm::vec3(origin.x + spread(gen), baseY, origin.z + spread(gen));
                p.velocity = glm::vec3(velocity(gen), -1.5f, velocity(gen));
                p.lifetime = particleLifetime;
                p.life = 1.0f;
                p.active = true;
                break;
            }
        }
    }
};
