
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <iostream>
#include <stdexcept>
#include "app.hpp"

int main() {
    try {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        json config = load_config();
        int width = config["window"]["width"];
        int height = config["window"]["height"];
        std::string title = config["window"]["title"];

        bool antialiasing_enabled;
        int samples;
        validate_antialiasing_settings(config, antialiasing_enabled, samples);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        if (antialiasing_enabled) {
            std::cout << "Antialiasing enabled, samples: " << samples << std::endl;
            glfwWindowHint(GLFW_SAMPLES, samples);
        }
        else {
            std::cout << "Antialiasing disabled" << std::endl;
            glfwWindowHint(GLFW_SAMPLES, 0);
        }

        GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(0);

        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            glfwTerminate();
            throw std::runtime_error(std::string("Failed to initialize GLEW: ") +
                (const char*)glewGetErrorString(err));
        }

        if (antialiasing_enabled) {
            glEnable(GL_MULTISAMPLE);
        }

        App myApp;
        if (!myApp.init(window)) {
            glfwTerminate();
            throw std::runtime_error("Failed to initialize application");
        }

        myApp.run();
        glfwTerminate();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
}