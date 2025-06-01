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

// app.cpp
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

    // Načtení textury pro ImGUI
    myTexture = textureInit("resources/my_favourite_texture.jpg");
    if (myTexture == 0) {
        std::cerr << "Failed to load texture for ImGUI" << std::endl;
    }
    else {
        std::cout << "Texture loaded: my_favourite_texture.jpg" << std::endl;
    }

    float tileSizeGL = 1.0f;
    float maxHeight = 20.0f;
    int width = heightmap.cols;
    int height = heightmap.rows;

    // Najdi nejvyšší bod v heightmapě
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

    // Uložení maximální výšky terénu
    maxTerrainHeight = maxValue / 255.0f * maxHeight;
    std::cout << "Max terrain height: " << maxTerrainHeight << std::endl;

    // Převod indexů na OpenGL souřadnice
    float startX = (static_cast<float>(max_hm_x) / width) * 15.0f * tileSizeGL;
    float startZ = (static_cast<float>(max_hm_z) / height) * 15.0f * tileSizeGL;
    float heightValue = maxValue / 255.0f * maxHeight;

    std::cout << "Start position: (" << startX << ", " << heightValue << ", " << startZ << ")" << std::endl;

    camera = Camera(glm::vec3(startX, heightValue + 0.5f, startZ));
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
    for (auto& bunny : transparent_bunnies) {
        delete bunny;
    }
    transparent_bunnies.clear();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Uklid ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

bool App::init(GLFWwindow* win) {
    window = win;
    fov = DEFAULT_FOV;

    if (!GLEW_ARB_direct_state_access) {
        std::cerr << "No DSA :-(" << std::endl;
        return false;
    }

    glfwSetWindowUserPointer(window, this);
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

    // Inicializace ImGUI
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
        std::cout << "Creating transparent bunnies..." << std::endl;
        createTransparentBunnies();
        std::cout << "Transparent bunnies created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Transparent bunnies creation error: " << e.what() << std::endl;
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

void App::createTransparentBunnies() {
    GLuint bunnyTexture = textureInit("resources/textures/kralik.jpg");
    std::vector<glm::vec3> bunny_positions = {
        glm::vec3(0.0f, 5.0f, 3.0f),
        glm::vec3(20.0f, 5.0f, 7.0f),
        glm::vec3(40.0f, 5.0f, 5.0f)
    };
    std::vector<glm::vec4> bunny_colors = {
        glm::vec4(1.0f, 0.3f, 0.3f, 0.8f),
        glm::vec4(0.3f, 1.0f, 0.3f, 0.6f),
        glm::vec4(0.3f, 0.3f, 1.0f, 0.4f)
    };

    for (int i = 0; i < 3; i++) {
        Model* bunny = new Model("resources/models/teapot_tri_vnt.obj", shader);
        bunny->meshes[0].texture_id = bunnyTexture;
        bunny->meshes[0].diffuse_material = bunny_colors[i];
        bunny->origin = bunny_positions[i];
        bunny->scale = glm::vec3(0.5f, 0.5f, 0.5f);
        bunny->transparent = true;
        transparent_bunnies.push_back(bunny);
    }
}

GLuint App::textureInit(const std::filesystem::path& filepath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Načtení obrázku pomocí OpenCV
    cv::Mat image = cv::imread(filepath.string(), cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    // Převod BGR (OpenCV) na RGB
    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

    // Nahrávání textury do OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Parametry textury
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
    float maxHeight = 20.0f; // Zvýšeno z 5.0 na 20.0

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
    // Vytvoření terénu
    createTerrainModel();

    // Vytvoření prázdné mapy (pro kompatibilitu s pohybem kamery)
    const int width = 15;
    const int height = 15;
    maze_map = cv::Mat(height, width, CV_8U, cv::Scalar('.')); // Všechny dlaždice jsou průchozí
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
    glm::vec3 lightPos(10.0f, maxTerrainHeight + 10.0f, 10.0f); // Y je nad maximální výškou terénu
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    shader.setUniform("lightPos", lightPos);
    shader.setUniform("lightColor", lightColor);
    shader.setUniform("viewPos", camera.Position);

    double lastTime = glfwGetTime();
    double lastFrameTime = lastTime;
    int frameCount = 0;
    std::string title = "PG2";
    static bool vsync = false; // Statická proměnná pro VSync
    json config = load_config(); // Načtení konfigurace
    bool antialiasing_enabled;
    int samples;
    validate_antialiasing_settings(config, antialiasing_enabled, samples); // Kontrola antialiasing
    bool isFullscreen = false; // Stav fullscreen režimu

    while (!glfwWindowShouldClose(window)) {
        // Inicializace ImGUI na začátku každého cyklu
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
        glm::vec3 direction = camera.ProcessKeyboard(window, deltaTime);
        camera.Move(direction, maze_map, 1.0f, heightmap, 20.0f);
        shader.setUniform("uV_m", camera.GetViewMatrix());
        shader.setUniform("viewPos", camera.Position);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& wall : maze_walls) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, wall->meshes[0].texture_id);
            shader.setUniform("tex0", 0);
            shader.setUniform("uM_m", wall->getModelMatrix());
            shader.setUniform("u_diffuse_color", wall->meshes[0].diffuse_material);
            wall->draw();
        }

        std::vector<Model*> transparent_objects;
        for (auto& bunny : transparent_bunnies) transparent_objects.push_back(bunny);
        std::sort(transparent_objects.begin(), transparent_objects.end(),
            [this](Model* a, Model* b) {
                float dist_a = glm::distance(camera.Position, a->origin);
                float dist_b = glm::distance(camera.Position, b->origin);
                return dist_a > dist_b;
            });

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        for (auto* model : transparent_objects) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model->meshes[0].texture_id);
            shader.setUniform("tex0", 0);
            shader.setUniform("uM_m", model->getModelMatrix());
            shader.setUniform("u_diffuse_color", model->meshes[0].diffuse_material);
            model->draw();
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // Monitorovací okno pomocí ImGUI
        ImGui::Begin("Monitoring", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Monitoring:");
        ImGui::Text("FPS: %d", frameCount);
        ImGui::Text("VSync: %s", vsync ? "ON" : "OFF");
        ImGui::Text("Antialiasing: %s (Samples: %d)", antialiasing_enabled ? "ON" : "OFF", samples);
        ImGui::Text("Mode: %s", isFullscreen ? "Fullscreen" : "Windowed");

        // Tlačítko pro přepínání fullscreen/windowed
        if (ImGui::Button("Toggle Fullscreen (F11)")) {
            isFullscreen = !isFullscreen;
            if (isFullscreen) {
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else {
                glfwSetWindowMonitor(window, nullptr, 100, 100, 800, 600, GLFW_DONT_CARE);
            }
        }
        ImGui::End();

        // Render ImGUI
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
    static bool vsync = false;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_F10:
            vsync = !vsync;
            glfwSwapInterval(vsync ? 1 : 0);
            std::cout << "VSync: " << (vsync ? "ON" : "OFF") << std::endl;
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
        }
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

void App::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        app->fov = app->DEFAULT_FOV;
        app->update_projection_matrix();
        std::cout << "Zoom reset to default" << std::endl;
    }
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