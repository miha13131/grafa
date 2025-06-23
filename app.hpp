#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp>
#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "assets.hpp"
#include "ShaderProgram.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Lights.hpp"
#include "ParticleSystem.hpp"

using json = nlohmann::json;

class App {
public:
    App();
    ~App();
    bool init();
    bool run();
    void init_glfw();
    static uchar getmap(cv::Mat& map, int x, int y);

private:
    GLFWwindow* window = nullptr;
    ShaderProgram shader;
    Model* triangle = nullptr;
    std::vector<Model*> maze_walls;
    std::vector<Model*> transparent_objects;
    std::vector<GLuint> transparent_textures;
    Camera camera;
    cv::Mat maze_map;
    int width = 800;
    int height = 600;
    int windowPosX = 100;
    int windowPosY = 100;
    int windowWidth = 800;
    int windowHeight = 600;
    double lastX, lastY;
    bool firstMouse;
    float fov{ 60.0f };
    const float DEFAULT_FOV = 60.0f;
    glm::mat4 projection_matrix;
    bool show_imgui = true;
    bool vsync = true;
    bool antialiasing_enabled = true; // New: Store MSAA enabled state
    int samples = 4;                 // New: Store MSAA sample count
    float r = 0.0f, g = 0.0f, b = 0.0f;
    Model* terrain;
    std::vector<Model*> models;
    std::vector<GLuint> model_textures;
    GLuint myTexture = 0;
    GLuint VAO = 0, VBO = 0;
    GLuint shaderProgram = 0;
    Lights lights;
    ParticleSystem particleSystem;

    void init_assets();
    void init_triangle();
    void createTerrainModel();
    void createMazeModel();
    void createModels();
    void createTransparentObjects();
    void initLights();
    GLuint textureInit(const std::filesystem::path& filepath);
    GLuint gen_tex(cv::Mat& image);
    void update_projection_matrix();
    static void fbsize_callback(GLFWwindow* window, int width, int height);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    void toggleFullscreen();
    static GLuint compileShader(GLenum type, const char* source);
};

void create_default_config();
json load_config();
bool validate_antialiasing_settings(const json& config, bool& antialiasing_enabled, int& samples);
void checkGLError(const std::string& context);