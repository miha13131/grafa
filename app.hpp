#pragma once
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

using json = nlohmann::json;

class App {
public:
    App();
    ~App();
    bool init(GLFWwindow* window);
    void init_assets();
    bool run();

    // Texture loading
    GLuint textureInit(const std::filesystem::path& filepath);
    GLuint myTexture; // Proměnná pro texturu je již deklarována

    // Callback methods
    static void fbsize_callback(GLFWwindow* window, int width, int height);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    cv::Mat loadHeightmap(const std::filesystem::path& filepath);
    void createTerrainModel();
    std::vector<float> heightmap_data; // vector of heightmap data
    Model* terrain; // Model for map
    void toggleFullscreen();
private:
    // Shader and model data
    ShaderProgram shader;
    Model* triangle{ nullptr };
    GLuint shaderProgram;
    GLuint VAO{ 0 }, VBO{ 0 }; // For the simple triangle

    // Maze data
    cv::Mat maze_map;
    cv::Mat heightmap;
    float maxTerrainHeight;
    std::vector<Model*> maze_walls;
    std::vector<GLuint> wall_textures;

    // Transparent bunnies (Task 1)
    std::vector<Model*> transparent_bunnies;
    void createTransparentBunnies();

    // Maze generation methods
    void genLabyrinth(cv::Mat& map);
    uchar getmap(cv::Mat& map, int x, int y);
    Model* createWall(ShaderProgram& shader, GLuint texture_id, glm::vec3 position, float height, glm::vec3 orientationAxis);
    Model* createSquareTile(ShaderProgram& shader, GLuint texture_id, glm::vec3 position);
    void createMazeModel();

    // Window and projection
    GLFWwindow* window{ nullptr };
    int width{ 800 }, height{ 600 };
    float fov{ 60.0f };
    const float DEFAULT_FOV = 60.0f;
    glm::mat4 projection_matrix{ glm::identity<glm::mat4>() };

    // Camera
    Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) };
    double lastX{ 400.0 }, lastY{ 300.0 };
    bool firstMouse{ true };

    // Triangle color
    float r{ 1.0f }, g{ 0.5f }, b{ 0.2f };

    // Utility methods
    void update_projection_matrix();
    GLuint gen_tex(cv::Mat& image);
    GLuint compileShader(GLenum type, const char* source);
    void init_triangle();
};

// Configuration loading and validation
void create_default_config();
json load_config();
bool validate_antialiasing_settings(const json& config, bool& antialiasing_enabled, int& samples);