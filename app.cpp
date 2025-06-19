#pragma warning(disable: 4005)

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "app.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <random>
#include <algorithm>

// Vertex shader for the simple triangle
const char* vertexShaderSource = R"(
    #version 460 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)";

// Fragment shader for the simple triangle
const char* fragmentShaderSource = R"(
    #version 460 core
    out vec4 FragColor;
    uniform vec3 triangleColor;
    void main() {
        FragColor = vec4(triangleColor, 1.0f);
    }
)";

// Utility function to check for OpenGL errors
void checkGLError(const std::string& context) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << context << ": " << err << std::endl;
    }
}

App::App() : lastX(400.0), lastY(300.0), firstMouse(true), fov(DEFAULT_FOV) {
    try {
        heightmap = loadHeightmap("resources/textures/heightmap.png");
        std::cout << "Heightmap loaded: " << heightmap.cols << "x" << heightmap.rows << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load heightmap in App constructor: " << e.what() << std::endl;
        heightmap = cv::Mat(15, 15, CV_8U, cv::Scalar(128));
        std::cout << "Using default heightmap: 15x15" << std::endl;
    }

    float tileSizeGL = 1.0f;
    float maxHeight = 20.0f;
    int width = heightmap.cols;
    int height = heightmap.rows;

    uchar maxValue = 0;
    int max_hm_x = 0;
    int max_hm_z = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uchar value = heightmap.at<uchar>(y, x);
            if (value > maxValue) {
                maxValue = value;
                max_hm_x = x;
                max_hm_z = y;
            }
        }
    }
    std::cout << "Max heightmap value: " << (int)maxValue << " at (" << max_hm_x << ", " << max_hm_z << ")" << std::endl;


    maxTerrainHeight = maxValue / 255.0f * maxHeight;
    std::cout << "Max terrain height: " << maxTerrainHeight << std::endl;

    float bunnyHeight = heightmap.at<uchar>(200, 200) / 255.0f * maxHeight;
    camera = Camera(glm::vec3(200.0f * tileSizeGL, bunnyHeight + 5.0f, 195.0f * tileSizeGL));

    // Initialize directional light (sun)
    directionalLight = Light{
        glm::vec3(200.0f, 100.0f, 200.0f), // Position (unused for directional)
        glm::vec3(-0.2f, -1.0f, -0.3f),   // Direction
        glm::vec3(0.7f),                   // Ambient
        glm::vec3(0.8f),                   // Diffuse
        glm::vec3(1.0f),                   // Specular
        1.0f, 0.0f, 0.0f,                  // Attenuation (unused)
        0.0f, 0.0f                        // Cutoff (unused)
    };

    // Initialize point lights
    pointLights.resize(3);
    pointLights[0] = Light{ // Near tree - green
        glm::vec3(110.0f, 30.0f, 110.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 0.2f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        1.0f, 0.001f, 0.00001f, // Reduced attenuation
        0.0f, 0.0f
    };
    pointLights[1] = Light{ // Near bunny - red
        glm::vec3(210.0f, 30.0f, 210.0f),
        glm::vec3(0.0f),
        glm::vec3(0.2f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        1.0f, 0.001f, 0.00001f, // Reduced attenuation
        0.0f, 0.0f
    };
    pointLights[2] = Light{ // Near house - blue
        glm::vec3(310.0f, 30.0f, 310.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 0.0f, 0.2f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        1.0f, 0.001f, 0.00001f, // Reduced attenuation
        0.0f, 0.0f
    };

    // Initialize spotlight (attached to camera)
    spotLight = Light{
        camera.Position,
        camera.Front,
        glm::vec3(0.1f),                   // Ambient
        glm::vec3(1.0f),                   // Diffuse
        glm::vec3(1.0f),                   // Specular
        1.0f, 0.01f, 0.001f,              // Reduced attenuation
        cos(glm::radians(20.0f)),          // Wider cutoff
        cos(glm::radians(25.0f))           // Wider outer cutoff
    };
}

App::~App() {
    shader.clear();
    if (triangle) {
        delete triangle;
        triangle = nullptr;
    }
    for (auto& wall : maze_walls) {
        delete wall;
    }
    maze_walls.clear();
    for (auto& obj : transparent_objects) {
        delete obj;
    }
    transparent_objects.clear();
    for (auto& model : models) {
        delete model;
    }
    models.clear();

    glDeleteTextures(1, &myTexture);
    glDeleteTextures(transparent_textures.size(), transparent_textures.data());
    transparent_textures.clear();
    glDeleteTextures(model_textures.size(), model_textures.data());
    model_textures.clear();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::init_glfw() {
    json config = load_config();
    bool antialiasing_enabled;
    int samples;
    validate_antialiasing_settings(config, antialiasing_enabled, samples);

    if (!glfwInit()) {
        throw std::runtime_error("GLFW can not be initialized.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (antialiasing_enabled) {
        glfwWindowHint(GLFW_SAMPLES, samples);
    }
    else {
        glfwWindowHint(GLFW_SAMPLES, 0);
    }

    int window_width = config["window"]["width"].get<int>();
    int window_height = config["window"]["height"].get<int>();
    std::string window_title = config["window"]["title"].get<std::string>();

    window = glfwCreateWindow(window_width, window_height, window_title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("GLFW window can not be created.");
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW can not be initialized.");
    }

    if (antialiasing_enabled) {
        glEnable(GL_MULTISAMPLE);
    }
}

bool App::init() {
    fov = DEFAULT_FOV;

    if (!GLEW_ARB_direct_state_access) {
        std::cerr << "No DSA :-(" << std::endl;
        return false;
    }

    glfwSetFramebufferSizeCallback(window, fbsize_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    init_assets();
    init_triangle();
    update_projection_matrix();

    if (shader.getID() != 0) {
        shader.activate();
        shader.setUniform("uV_m", camera.GetViewMatrix());
    }

    return true;
}

void App::init_assets() {
    myTexture = textureInit("resources/textures/grass.png");
    if (myTexture == 0) {
        std::cerr << "Failed to load texture for ImGUI" << std::endl;
    }
    else {
        std::cout << "Texture loaded: grass.png" << std::endl;
    }

    try {
        std::cout << "Loading shaders..." << std::endl;
        shader = ShaderProgram("resources/shaders/tex.vert", "resources/shaders/tex.frag");
        std::cout << "Shaders loaded successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Shader loading error: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "Creating terrain..." << std::endl;
        createMazeModel();
        std::cout << "Terrain created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Terrain creation error: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "Creating transparent objects..." << std::endl;
        createTransparentObjects();
        std::cout << "Transparent objects created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Transparent objects creation error: " << e.what() << std::endl;
        throw;
    }

    try {
        std::cout << "Creating models..." << std::endl;
        createModels();
        std::cout << "Models created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Models creation error: " << e.what() << std::endl;
        throw;
    }
}

void App::init_triangle() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void App::createTransparentObjects() {
    // Clear previous transparent objects and textures
    for (auto& obj : transparent_objects) {
        delete obj;
    }
    transparent_objects.clear();
    transparent_textures.clear();

    // Načtení textury kralik.jpg
    GLuint objectTexture = textureInit("resources/textures/kralik.jpg");
    if (objectTexture == 0) {
        std::cerr << "Failed to load texture kralik.jpg for transparent objects" << std::endl;
    }
    else {
        transparent_textures.push_back(objectTexture);
    }

    // Terrain dimensions
    int width = heightmap.cols;
    int height = heightmap.rows;
    float tileSizeGL = 1.0f;
    float maxHeight = 20.0f;

    // Fixed positions for three objects
    std::vector<glm::vec3> positions = {
        glm::vec3(100.0f * tileSizeGL, heightmap.at<uchar>(100, 100) / 255.0f * maxHeight + 1.0f, 100.0f * tileSizeGL), // Tree
        glm::vec3(200.0f * tileSizeGL, heightmap.at<uchar>(200, 200) / 255.0f * maxHeight + 2.0f, 200.0f * tileSizeGL), // Bunny
        glm::vec3(300.0f * tileSizeGL, heightmap.at<uchar>(300, 300) / 255.0f * maxHeight + 1.0f, 300.0f * tileSizeGL)  // House
    };

    // Colors with alpha for transparency
    std::vector<glm::vec4> colors = {
        glm::vec4(0.3f, 1.0f, 0.3f, 0.8f), // Tree - zelený
        glm::vec4(1.0f, 0.3f, 0.3f, 0.6f), // Bunny - červený
        glm::vec4(0.3f, 0.3f, 1.0f, 0.4f)  // House - modrý
    };

    // Model paths
    std::vector<std::string> modelPaths = {
        "resources/models/tree.obj",
        "resources/models/bunny_tri_vnt.obj",
        "resources/models/house.obj"
    };

    // Scales for each object
    std::vector<glm::vec3> scales = {
        glm::vec3(1.0f, 1.0f, 1.0f), // Tree
        glm::vec3(1.0f, 1.0f, 1.0f), // Bunny
        glm::vec3(1.0f, 1.0f, 1.0f)  // House
    };

    // Create models with fixed scale and apply texture
    for (int i = 0; i < 3; i++) {
        Model* model = new Model(modelPaths[i], shader);
        if (!model->meshes.empty()) {
            model->meshes[0].texture_id = objectTexture; // Použití textury
            model->meshes[0].diffuse_material = colors[i];
        }
        model->origin = positions[i];
        model->scale = scales[i];
        model->transparent = true;
        transparent_objects.push_back(model);
        std::cout << "Placed transparent object " << i << " at position ("
            << positions[i].x << ", " << positions[i].y << ", " << positions[i].z << ")\n";
    }
}

void App::createModels() {
    // Clear previous models and textures
    for (auto* model : models) {
        delete model;
    }
    models.clear();
    model_textures.clear();

    // Načtení textur
    std::vector<std::string> texturePaths = {
        "resources/textures/krabice.jpg", // Pro krychli
        "resources/textures/Cat.jpg",     // Pro kočku
        "resources/textures/Tractor.jpg"  // Pro traktor
    };

    for (const auto& path : texturePaths) {
        GLuint modelTexture = textureInit(path);
        if (modelTexture == 0) {
            std::cerr << "Failed to load texture " << path << " for models" << std::endl;
        }
        else {
            model_textures.push_back(modelTexture);
            std::cout << "Successfully loaded texture: " << path << std::endl;
        }
    }

    // Terrain dimensions
    int width = heightmap.cols;
    int height = heightmap.rows;
    float tileSizeGL = 1.0f;
    float maxHeight = 20.0f;

    // Fixed positions for three models, all at x = 150, varying z
    std::vector<glm::vec3> positions = {
        glm::vec3(150.0f * tileSizeGL, heightmap.at<uchar>(150, 150) / 255.0f * maxHeight + 1.0f, 150.0f * tileSizeGL), // Cube
        glm::vec3(150.0f * tileSizeGL, heightmap.at<uchar>(150, 160) / 255.0f * maxHeight + 1.0f, 160.0f * tileSizeGL), // Cat
        glm::vec3(150.0f * tileSizeGL, heightmap.at<uchar>(150, 170) / 255.0f * maxHeight + 1.0f, 170.0f * tileSizeGL)  // Tractor
    };

    // Colors with alpha = 1.0 for opacity, neutral for all to use only texture
    std::vector<glm::vec4> colors = {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // Cube - neutrální barva, pouze textura
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // Cat - neutrální barva, pouze textura
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // Tractor - neutrální barva, pouze textura
    };

    // Model paths
    std::vector<std::string> modelPaths = {
        "resources/models/cube.obj",
        "resources/models/cat.obj",
        "resources/models/Tractor.obj"
    };

    // Scales for each model, smaller for cat and tractor
    std::vector<glm::vec3> scales = {
        glm::vec3(1.0f, 1.0f, 1.0f),  // Cube
        glm::vec3(0.5f, 0.5f, 0.5f),  // Cat - zmenšen na polovinu
        glm::vec3(0.7f, 0.7f, 0.7f)   // Tractor - zmenšen na 70%
    };

    // Create models with fixed scale and apply texture
    for (int i = 0; i < 3; i++) {
        Model* model = new Model(modelPaths[i], shader);
        if (!model->meshes.empty()) {
            model->meshes[0].texture_id = model_textures[i]; // Použití odpovídající textury
            model->meshes[0].diffuse_material = colors[i];
        }
        model->origin = positions[i];
        model->scale = scales[i];
        model->transparent = false; // Neprůhledné modely

        // Převrátit kočku kolem osy y (rotace o 180 stupňů)
        if (i == 1) { // Cat je druhý model (index 1)
            model->orientation = glm::vec3(0.0f, glm::radians(180.0f), 0.0f);
        }

        models.push_back(model);
        std::cout << "Placed model " << i << " at position ("
            << positions[i].x << ", " << positions[i].y << ", " << positions[i].z << ")\n";
    }
}

GLuint App::textureInit(const std::filesystem::path& filepath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    checkGLError("After glBindTexture in textureInit");

    cv::Mat image = cv::imread(filepath.string(), cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &textureID);
        return 0;
    }

    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
    checkGLError("After glTexImage2D in textureInit");

    glGenerateMipmap(GL_TEXTURE_2D);
    checkGLError("After glGenerateMipmap in textureInit");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

GLuint App::gen_tex(cv::Mat& image) {
    GLuint ID = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);
    switch (image.channels()) {
    case 3:
        glTextureStorage2D(ID, 1, GL_RGB8, image.cols, image.rows);
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGR, GL_UNSIGNED_BYTE, image.data);
        break;
    case 4:
        glTextureStorage2D(ID, 1, GL_RGBA8, image.cols, image.rows);
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGRA, GL_UNSIGNED_BYTE, image.data);
        break;
    default:
        throw std::runtime_error("Unsupported number of channels in texture: " + std::to_string(image.channels()));
    }
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateTextureMipmap(ID);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return ID;
}

cv::Mat App::loadHeightmap(const std::filesystem::path& filepath) {
    std::cout << "Loading heightmap: " << filepath << std::endl;
    cv::Mat heightmap = cv::imread(filepath.string(), cv::IMREAD_GRAYSCALE);
    if (heightmap.empty()) {
        throw std::runtime_error("Failed to load heightmap from file: " + filepath.string());
    }
    return heightmap;
}

void App::createTerrainModel() {
    if (heightmap.empty()) {
        throw std::runtime_error("Heightmap is empty in createTerrainModel");
    }
    int width = heightmap.cols;
    int height = heightmap.rows;
    float tileSizeGL = 1.0f;
    float maxHeight = 20.0f;

    GLuint terrainTexture = textureInit("resources/textures/grass.png");

    std::vector<vertex> vertices;
    std::vector<GLuint> indices;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float heightValue = heightmap.at<uchar>(y, x) / 255.0f * maxHeight;
            glm::vec3 position(x * tileSizeGL, heightValue, y * tileSizeGL);
            glm::vec2 texCoord(static_cast<float>(x) / (width - 1), static_cast<float>(y) / (height - 1));
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            vertices.push_back(vertex{ position, texCoord, normal });
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            float hL = (x > 0) ? heightmap.at<uchar>(y, x - 1) / 255.0f * maxHeight : heightmap.at<uchar>(y, x) / 255.0f * maxHeight;
            float hR = (x < width - 1) ? heightmap.at<uchar>(y, x + 1) / 255.0f * maxHeight : heightmap.at<uchar>(y, x) / 255.0f * maxHeight;
            float hU = (y > 0) ? heightmap.at<uchar>(y - 1, x) / 255.0f * maxHeight : heightmap.at<uchar>(y, x) / 255.0f * maxHeight;
            float hD = (y < height - 1) ? heightmap.at<uchar>(y + 1, x) / 255.0f * maxHeight : heightmap.at<uchar>(y, x) / 255.0f * maxHeight;
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hU - hD));
            vertices[idx].normal = normal;
        }
    }

    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = y * width + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * width + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    terrain = new Model("", shader);
    Mesh mesh(GL_TRIANGLES, shader, vertices, indices, glm::vec3(0.0f), glm::vec3(0.0f));
    mesh.texture_id = terrainTexture;
    mesh.diffuse_material = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    terrain->meshes.push_back(mesh);
    terrain->origin = glm::vec3(0.0f);
    terrain->scale = glm::vec3(1.0f);
    terrain->orientation = glm::vec3(0.0f);

    maze_walls.push_back(terrain);
}

void App::createMazeModel() {
    createTerrainModel();

    const int width = 15;
    const int height = 15;
    maze_map = cv::Mat(height, width, CV_8U, cv::Scalar('.'));
}

uchar App::getmap(cv::Mat& map, int x, int y) {
    x = std::clamp(x, 0, map.cols - 1);
    y = std::clamp(y, 0, map.rows - 1);
    return map.at<uchar>(y, x);
}

bool App::run() {
    if (!window) {
        std::cerr << "No active GLFW window!" << std::endl;
        return false;
    }
    if (fov <= 0.0f) fov = DEFAULT_FOV;

    shader.activate();
    update_projection_matrix();
    shader.setUniform("uV_m", camera.GetViewMatrix());
    shader.setUniform("uP_m", projection_matrix);
    shader.setUniform("viewPos", camera.Position);

    double lastTime = glfwGetTime();
    double lastFrameTime = lastTime;
    int frameCount = 0;
    std::string title = "PG2";

    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastFrameTime);
        lastFrameTime = currentTime;
        frameCount++;

        if (currentTime - lastTime >= 1.0) {
            std::string fpsTitle = title + " | FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, fpsTitle.c_str());
            frameCount = 0;
            lastTime = currentTime;
        }

        // Activate shader and set uniforms
        shader.activate();
        shader.setUniform("ambientLight.color", glm::vec3(0.2f)); // Set ambient light

        // Update directional light
        float sunAngle = currentTime * 0.1f;
        directionalLight.direction = glm::vec3(sin(sunAngle) * 0.5f, -1.0f, cos(sunAngle) * 0.5f);
        directionalLight.ambient = glm::vec3(0.7f);
        directionalLight.diffuse = glm::vec3(0.9f + 0.1f * sin(sunAngle), 0.9f + 0.1f * cos(sunAngle), 0.9f);
        directionalLight.specular = glm::vec3(1.0f);
        shader.setUniform("dirLights[0].direction", directionalLight.direction);
        shader.setUniform("dirLights[0].ambient", directionalLight.ambient);
        shader.setUniform("dirLights[0].diffuse", directionalLight.diffuse);
        shader.setUniform("dirLights[0].specular", directionalLight.specular);
        std::cout << "DirLight dir: " << directionalLight.direction.x << ", " << directionalLight.direction.y << ", " << directionalLight.direction.z << std::endl;

        // Update point lights
        shader.setUniform("numPointLights", 3);
        for (int i = 0; i < 3; i++) {
            float intensity = 0.7f + 0.3f * sin(currentTime * (i + 1));
            pointLights[i].diffuse = pointLights[i].diffuse * intensity;
            pointLights[i].specular = pointLights[i].diffuse;
            std::string index = "[" + std::to_string(i) + "]";
            shader.setUniform("pointLights" + index + ".position", pointLights[i].position);
            shader.setUniform("pointLights" + index + ".ambient", pointLights[i].ambient);
            shader.setUniform("pointLights" + index + ".diffuse", pointLights[i].diffuse);
            shader.setUniform("pointLights" + index + ".specular", pointLights[i].specular);
            shader.setUniform("pointLights" + index + ".constant", pointLights[i].constant);
            shader.setUniform("pointLights" + index + ".linear", pointLights[i].linear);
            shader.setUniform("pointLights" + index + ".quadratic", pointLights[i].quadratic);
            std::cout << "PointLight[" << i << "] pos: " << pointLights[i].position.x << ", " << pointLights[i].position.y << ", " << pointLights[i].position.z << std::endl;
        }

        // Update spotlight
        shader.setUniform("numSpotLights", 1);
        spotLight.position = camera.Position;
        spotLight.direction = camera.Front;
        shader.setUniform("spotLights[0].position", spotLight.position);
        shader.setUniform("spotLights[0].direction", spotLight.direction);
        shader.setUniform("spotLights[0].ambient", spotLight.ambient);
        shader.setUniform("spotLights[0].diffuse", spotLight.diffuse);
        shader.setUniform("spotLights[0].specular", spotLight.specular);
        shader.setUniform("spotLights[0].constant", spotLight.constant);
        shader.setUniform("spotLights[0].linear", spotLight.linear);
        shader.setUniform("spotLights[0].quadratic", spotLight.quadratic);
        shader.setUniform("spotLights[0].cutOff", spotLight.cutOff);
        shader.setUniform("spotLights[0].outerCutOff", spotLight.outerCutOff);
        std::cout << "SpotLight pos: " << spotLight.position.x << ", " << spotLight.position.y << ", " << spotLight.position.z << std::endl;

        // Camera movement
        glm::vec3 direction = camera.ProcessKeyboard(window, deltaTime); 
        camera.Move(direction, maze_map, 1.0f, heightmap, 20.0f, deltaTime);
        shader.setUniform("uV_m", camera.GetViewMatrix());
        shader.setUniform("viewPos", camera.Position);
        std::cout << "Camera pos: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << std::endl;

        // Rendering
        glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render opaque objects
        for (auto& wall : maze_walls) {
            if (!wall->transparent) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, wall->meshes[0].texture_id);
                shader.setUniform("tex0", 0);
                shader.setUniform("uM_m", wall->getModelMatrix());
                // Remove u_diffuse_color as it's not used in tex.frag
                wall->draw();
                checkGLError("After drawing wall");
            }
        }

        // Render models
        for (auto& model : models) {
            if (!model->transparent) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, model->meshes[0].texture_id);
                shader.setUniform("tex0", 0);
                shader.setUniform("uM_m", model->getModelMatrix());
                model->draw();
                checkGLError("After drawing model");
            }
        }

        // Render transparent objects
        std::vector<Model*> transparent_draw_list;
        for (auto& wall : maze_walls) {
            if (wall->transparent) {
                transparent_draw_list.push_back(wall);
            }
        }
        for (auto& obj : transparent_objects) {
            transparent_draw_list.push_back(obj);
        }
        for (auto& model : models) {
            if (model->transparent) {
                transparent_draw_list.push_back(model);
            }
        }

        std::sort(transparent_draw_list.begin(), transparent_draw_list.end(),
            [this](Model* a, Model* b) {
                float dist_a = glm::distance(camera.Position, a->origin);
                float dist_b = glm::distance(camera.Position, b->origin);
                return dist_a > dist_b;
            });

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        for (auto* model : transparent_draw_list) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model->meshes[0].texture_id);
            shader.setUniform("tex0", 0);
            shader.setUniform("uM_m", model->getModelMatrix());
            model->draw();
            checkGLError("After drawing transparent model");
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // ImGui rendering
        if (show_imgui) {
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowSize(ImVec2(250, 100));
            ImGui::Begin("Monitoring", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Text("V-Sync: %s", vsync ? "ON" : "OFF");
            ImGui::Text("FPS: %d", frameCount);
            ImGui::Text("(press RMB to release mouse)");
            ImGui::Text("(press H to show/hide info)");
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    return true;
}

void App::update_projection_matrix() {
    if (height < 1) height = 1;
    if (fov <= 0.0f) fov = DEFAULT_FOV;

    float ratio = static_cast<float>(width) / height;
    projection_matrix = glm::perspective(glm::radians(fov), ratio, 0.1f, 20000.0f);

    if (shader.getID() != 0) {
        shader.activate();
        shader.setUniform("uP_m", projection_matrix);
    }
}

GLuint App::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

void App::fbsize_callback(GLFWwindow* window, int width, int height) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->width = width;
    app->height = height;
    glViewport(0, 0, width, height);
    app->update_projection_matrix();
}

void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->fov += -5.0f * static_cast<float>(yoffset);
    app->fov = glm::clamp(app->fov, 20.0f, 170.0f);
    app->update_projection_matrix();
}

void App::toggleFullscreen() {
    bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
    if (isFullscreen) {
        glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, GLFW_DONT_CARE);
    }
    else {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_F10:
            app->vsync = !app->vsync;
            glfwSwapInterval(app->vsync ? 1 : 0);
            std::cout << "VSync: " << (app->vsync ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_F11:
            app->toggleFullscreen();
            break;
        case GLFW_KEY_R:
            app->r += 0.1f;
            if (app->r > 1.0f) app->r = 0.0f;
            break;
        case GLFW_KEY_G:
            app->g += 0.1f;
            if (app->g > 1.0f) app->g = 0.0f;
            break;
        case GLFW_KEY_B:
            app->b += 0.1f;
            if (app->b > 1.0f) app->b = 0.0f;
            break;
        case GLFW_KEY_H:
            app->show_imgui = !app->show_imgui;
            if (app->show_imgui) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            break;
        }
    }
}

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        app->firstMouse = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        app->fov = app->DEFAULT_FOV;
        app->update_projection_matrix();
        std::cout << "Zoom reset to default" << std::endl;
    }
}

void App::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app->firstMouse) {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }
    double xoffset = xpos - app->lastX;
    double yoffset = app->lastY - ypos;
    app->lastX = xpos;
    app->lastY = ypos;
    app->camera.ProcessMouseMovement(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

void create_default_config() {
    json config;
    config["window"] = {
        {"width", 800},
        {"height", 600},
        {"title", "OpenGL Maze Demo"}
    };
    config["graphics"] = {
        {"antialiasing", {
            {"enabled", false},
            {"samples", 4}
        }}
    };
    std::ofstream file("config.json");
    if (!file.is_open()) {
        std::cerr << "Failed to create config file!" << std::endl;
        return;
    }
    file << config.dump(4);
}

json load_config() {
    try {
        std::ifstream file("config.json");
        if (!file.is_open()) {
            std::cout << "Config file does not exist, creating default." << std::endl;
            create_default_config();
            file.open("config.json");
            if (!file.is_open()) {
                throw std::runtime_error("Failed to create config file");
            }
        }
        return json::parse(file);
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        json default_config;
        default_config["window"] = {
            {"width", 800},
            {"height", 600},
            {"title", "OpenGL Maze Demo"}
        };
        default_config["graphics"] = {
            {"antialiasing", {
                {"enabled", false},
                {"samples", 4}
            }}
        };
        return default_config;
    }
}

bool validate_antialiasing_settings(const json& config, bool& antialiasing_enabled, int& samples) {
    bool valid = true;
    if (!config.contains("graphics") || !config["graphics"].contains("antialiasing")) {
        std::cerr << "Warning: Antialiasing settings missing in config." << std::endl;
        antialiasing_enabled = false;
        samples = 0;
        return false;
    }
    const auto& aa_config = config["graphics"]["antialiasing"];
    antialiasing_enabled = aa_config.value("enabled", false);
    samples = aa_config.value("samples", 0);
    if (antialiasing_enabled) {
        if (samples <= 1) {
            std::cerr << "Warning: Antialiasing enabled but samples <= 1. Setting to 4." << std::endl;
            samples = 4;
            valid = false;
        }
        else if (samples > 8) {
            std::cerr << "Warning: Too many antialiasing samples (> 8). Setting to 8." << std::endl;
            samples = 8;
            valid = false;
        }
    }
    return valid;
}