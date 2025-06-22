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

bool AABBintersect(const glm::vec3& minA, const glm::vec3& maxA,
    const glm::vec3& minB, const glm::vec3& maxB) {
    return (minA.x <= maxB.x && maxA.x >= minB.x) &&
        (minA.y <= maxB.y && maxA.y >= minB.y) &&
        (minA.z <= maxB.z && maxA.z >= minB.z);
}

App::App() : lastX(400.0), lastY(300.0), firstMouse(true), fov(DEFAULT_FOV) {
    camera = Camera(glm::vec3(160.0f, 12.0f, 160.0f));
    // Initialize lights via Lights class
    lights.initAmbientLight(glm::vec3(0.2f));
    lights.initDirectionalLight();
    lights.initPointLight(glm::vec3(100.0f, 30.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Green, near tree
    lights.initPointLight(glm::vec3(75.0f, 30.0f, 25.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Red, near bunny
    lights.initPointLight(glm::vec3(75.0f, 30.0f, 100.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Blue, near house
    lights.initSpotLight(camera.Position, camera.Front); // Camera-attached spotlight
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
    glfwDestroyWindow(window);
    glfwTerminate();
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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

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

    initLights();
}

void App::initLights() {
    std::filesystem::path point_lights_path = "resources/lights/point_lights.lights";
    std::ifstream file_point_light(point_lights_path);
    if (file_point_light.is_open()) {
        std::string line;
        while (std::getline(file_point_light, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            float x, y, z, r, g, b;
            if (!(ss >> x >> y >> z >> r >> g >> b)) {
                std::cerr << "Invalid point light entry: " << line << std::endl;
                continue;
            }
            lights.initPointLight(glm::vec3(x, y, z), glm::vec3(r, g, b));
        }
        file_point_light.close();
    }
    else {
        std::cout << "Using default point lights" << std::endl;
        // Default point lights already initialized in constructor
    }

    std::filesystem::path spot_lights_path = "resources/lights/spot_lights.lights";
    std::ifstream file_spot_light(spot_lights_path);
    if (file_spot_light.is_open()) {
        std::string line;
        while (std::getline(file_spot_light, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            float posX, posY, posZ, dirX, dirY, dirZ;
            if (!(ss >> posX >> posY >> posZ >> dirX >> dirY >> dirZ)) {
                std::cerr << "Invalid spot light entry: " << line << std::endl;
                continue;
            }
            lights.initSpotLight(glm::vec3(posX, posY, posZ), glm::vec3(dirX, dirY, dirZ));
        }
        file_spot_light.close();
    }
    else {
        std::cout << "Using default spotlight" << std::endl;
        // Default spotlight already initialized in constructor
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

    // Load textures
    std::vector<std::string> texturePaths = {
        "resources/textures/Green.png",  // For tree
        "resources/textures/red.jpg",    // For bunny
        "resources/textures/blue.png"    // For house
    };
    std::vector<GLuint> objectTextures;

    for (size_t i = 0; i < texturePaths.size(); ++i) {
        GLuint texture = textureInit(texturePaths[i]);
        if (texture == 0) {
            std::cerr << "Failed to load texture: " << texturePaths[i] << std::endl;
        }
        else {
            objectTextures.push_back(texture);
            transparent_textures.push_back(texture);
            std::cout << "Loaded texture: " << texturePaths[i] << std::endl;
        }
    }

    float tileSizeGL = 1.0f;

    // Positions on flat plane
    std::vector<glm::vec3> positions = {
        glm::vec3(100.0f * tileSizeGL, 0.01f, 50.0f * tileSizeGL),  // Tree
        glm::vec3(75.0f * tileSizeGL, 2.0f, 25.0f * tileSizeGL),    // Bunny
        glm::vec3(75.0f * tileSizeGL, 0.01f, 100.0f * tileSizeGL)   // House
    };

    // Colors with alpha for transparency
    std::vector<glm::vec4> colors = {
        glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), // Tree - green
        glm::vec4(1.0f, 1.0f, 1.0f, 0.6f), // Bunny - red
        glm::vec4(1.0f, 1.0f, 1.0f, 0.4f)  // House - blue
    };

    // Model paths
    std::vector<std::string> modelPaths = {
        "resources/models/tree.obj",
        "resources/models/bunny_tri_vnt.obj",
        "resources/models/Barrel_OBJ.obj"
    };

    // Scales for each object
    std::vector<glm::vec3> scales = {
        glm::vec3(5.0f, 5.0f, 5.0f),   // Tree
        glm::vec3(4.0f, 4.0f, 4.0f),   // Bunny
        glm::vec3(10.0f, 10.0f, 10.0f) // House
    };

    // Create models with fixed scale and apply texture
    for (int i = 0; i < 3; i++) {
        Model* model = new Model(modelPaths[i], shader);
        if (!model->meshes.empty() && i < objectTextures.size()) {
            model->meshes[0].texture_id = objectTextures[i];
            model->meshes[0].diffuse_material = colors[i];
        }
        else {
            std::cerr << "Warning: No texture assigned to model " << modelPaths[i] << std::endl;
            if (!model->meshes.empty()) {
                model->meshes[0].diffuse_material = colors[i];
            }
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

    // Load textures
    std::vector<std::string> texturePaths = {
        "resources/textures/krabice.jpg", // Cube
        "resources/textures/Cat.jpg",     // Cat
        "resources/textures/Tractor.jpg", // Tractor
        "resources/textures/barrel.png"   // House (Barrel model)
    };

    for (const auto& path : texturePaths) {
        GLuint modelTexture = textureInit(path);
        if (modelTexture == 0) {
            std::cerr << "Failed to load texture " << path << std::endl;
        }
        else {
            model_textures.push_back(modelTexture);
            std::cout << "Successfully loaded texture: " << path << std::endl;
        }
    }

    float tileSizeGL = 1.0f;

    // Positions on flat plane
    std::vector<glm::vec3> positions = {
        glm::vec3(1.0f * tileSizeGL, 0.5f, 1.0f * tileSizeGL),      // Cube
        glm::vec3(50.0f * tileSizeGL, 0.01f, 0.0f * tileSizeGL),    // Cat
        glm::vec3(75.0f * tileSizeGL, 40.0f, 75.0f * tileSizeGL),   // Tractor
        glm::vec3(75.0f * tileSizeGL, 0.01f, 100.0f * tileSizeGL)   // House (already in transparent_objects)
    };

    // Colors with alpha = 1.0 for opacity
    std::vector<glm::vec4> colors = {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // Cube
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // Cat
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // Tractor
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // House
    };

    // Model paths
    std::vector<std::string> modelPaths = {
        "resources/models/cube.obj",
        "resources/models/cat.obj",
        "resources/models/Tractor.obj",
        "resources/models/Barrel_OBJ.obj"
    };

    // Scales for each model
    std::vector<glm::vec3> scales = {
        glm::vec3(10.0f, 10.0f, 10.0f), // Cube
        glm::vec3(0.3f, 0.3f, 0.3f),    // Cat
        glm::vec3(0.5f, 0.5f, 0.5f),    // Tractor
        glm::vec3(10.0f, 10.0f, 10.0f)  // House
    };

    // Create models with fixed scale and apply texture
    for (int i = 0; i < 3; i++) { // Exclude house (index 3), as it's in transparent_objects
        Model* model = new Model(modelPaths[i], shader);
        if (i == 1) { // Cat
            model->orientation = glm::vec3(glm::radians(270.0f), 0.0f, 0.0f);
        }
        if (i == 2) { // Tractor
            model->orientation = glm::vec3(0.0f, glm::radians(30.0f), 0.0f);
        }
        if (!model->meshes.empty()) {
            model->meshes[0].texture_id = model_textures[i];
            model->meshes[0].diffuse_material = colors[i];
        }
        model->origin = positions[i];
        model->scale = scales[i];
        model->transparent = false;
        models.push_back(model);
        std::cout << "Placed model " << i << " at position ("
            << positions[i].x << ", " << positions[i].y << ", " << positions[i].z << ")\n";
    }
}

GLuint App::textureInit(const std::filesystem::path& filepath) {
    cv::Mat image = cv::imread(filepath.string(), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return 0;
    }
    GLuint textureID = gen_tex(image);
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

void App::createTerrainModel() {
    GLuint terrainTexture = textureInit("resources/textures/grass.png");
    terrain = new Model("resources/models/plane_tri_vnt.obj", shader);
    if (!terrain->meshes.empty()) {
        terrain->meshes[0].texture_id = terrainTexture;
        terrain->meshes[0].diffuse_material = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    terrain->origin = glm::vec3(0.0f, 0.0f, 0.0f);
    terrain->scale = glm::vec3(400.0f, 1.0f, 400.0f);
    terrain->orientation = glm::vec3(0.0f);
    maze_walls.push_back(terrain);
}

void App::createMazeModel() {
    createTerrainModel();
    const int width = 400;
    const int height = 400;
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
            std::string fpsTitle = title + " | FPS: " + std::to_string(frameCount) + ", VSYNC: " + (vsync ? "ON" : "OFF");
            glfwSetWindowTitle(window, fpsTitle.c_str());
            frameCount = 0;
            lastTime = currentTime;
        }

        shader.activate();
        // Update lights (day/night cycle and movement)
        float sunAngle = currentTime * 0.3f;
        float daylight = glm::clamp(sin(sunAngle), 0.0f, 1.0f);
        float smoothDay = daylight * daylight;

        lights.ambientLight.color = glm::vec3(0.2f, 0.2f, 0.25f) + glm::vec3(0.5f, 0.5f, 0.4f) * smoothDay;
        lights.sun.direction = glm::normalize(glm::vec3(cos(sunAngle), -0.5f, sin(sunAngle)));
        lights.sun.ambient = glm::vec3(0.2f) + glm::vec3(0.1f) * smoothDay;
        lights.sun.diffuse = glm::vec3(0.5f) + glm::vec3(0.3f) * smoothDay;
        lights.sun.specular = glm::vec3(1.0f);

        // Move point light[0] (green, near tree): Circular with vertical oscillation
        lights.pointLights[0].position = glm::vec3(
            100.0f + 10.0f * sin(currentTime),      // Circular in X
            30.0f + 5.0f * cos(currentTime * 0.5f), // Oscillate in Y
            50.0f + 10.0f * cos(currentTime)        // Circular in Z
        );
        float intensity0 = 0.7f + 0.3f * sin(currentTime);
        lights.pointLights[0].diffuse = glm::vec3(0.0f, 1.0f, 0.0f) * intensity0;
        lights.pointLights[0].specular = lights.pointLights[0].diffuse;

        // Move point light[1] (red, near bunny): Elliptical in XZ plane
        lights.pointLights[1].position = glm::vec3(
            75.0f + 12.0f * cos(currentTime * 0.4f), // Elliptical in X
            30.0f,                                   // Fixed height
            25.0f + 8.0f * sin(currentTime * 0.4f)   // Elliptical in Z
        );
        float intensity1 = 0.7f + 0.3f * cos(currentTime * 0.4f);
        lights.pointLights[1].diffuse = glm::vec3(1.0f, 0.0f, 0.0f) * intensity1;
        lights.pointLights[1].specular = lights.pointLights[1].diffuse;

        // Move point light[2] (blue, near house): Spiral upward/downward
        float spiralHeight = 30.0f + 10.0f * sin(currentTime * 0.3f);
        lights.pointLights[2].position = glm::vec3(
            75.0f + 10.0f * cos(currentTime * 0.6f), // Circular in X
            spiralHeight,                            // Spiral in Y
            100.0f + 10.0f * sin(currentTime * 0.6f) // Circular in Z
        );
        float intensity2 = 0.7f + 0.3f * sin(currentTime * 0.6f);
        lights.pointLights[2].diffuse = glm::vec3(0.0f, 0.0f, 1.0f) * intensity2;
        lights.pointLights[2].specular = lights.pointLights[2].diffuse;

        // Update spotlight (camera-attached)
        lights.spotLights[0].position = camera.Position;
        lights.spotLights[0].direction = camera.Front;

        lights.apply(shader.getID());

        // Camera movement with collision detection
        glm::vec3 direction = camera.ProcessKeyboard(window, deltaTime);
        glm::vec3 newPos = camera.Position + direction * deltaTime;

        // Apply gravity and jumping physics
        camera.VerticalVelocity += camera.Gravity * deltaTime;
        newPos.y += camera.VerticalVelocity * deltaTime;
        if (newPos.y <= 12.0f) {
            newPos.y = 12.0f;
            camera.VerticalVelocity = 0.0f;
            camera.OnGround = true;
        }
        glm::vec3 cameraMin = newPos - glm::vec3(0.5f, 1.0f, 0.5f);
        glm::vec3 cameraMax = newPos + glm::vec3(0.5f, 1.0f, 0.5f);

        bool collision = false;
        for (const auto* model : models) {
            if (AABBintersect(cameraMin, cameraMax, model->getMinBounds(), model->getMaxBounds())) {
                collision = true;
                break;
            }
        }
        for (const auto* model : transparent_objects) {
            if (AABBintersect(cameraMin, cameraMax, model->getMinBounds(), model->getMaxBounds())) {
                collision = true;
                break;
            }
        }
        if (!collision) {
            camera.Position = newPos;
        }

        shader.setUniform("uV_m", camera.GetViewMatrix());
        shader.setUniform("viewPos", camera.Position);

        for (auto& model : models) {
            model->update(deltaTime);
        }

        glClearColor(0.3f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& wall : maze_walls) {
            if (!wall->transparent) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, wall->meshes[0].texture_id);
                shader.setUniform("tex0", 0);
                shader.setUniform("uM_m", wall->getModelMatrix());
                wall->draw();
                checkGLError("After drawing wall");
            }
        }

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