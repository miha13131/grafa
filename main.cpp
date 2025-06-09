#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <iostream>
#include <stdexcept>
#include "app.hpp"


int main() {
    try {
        App myApp;
        myApp.init_glfw(); // Inicializace GLFW a okna je nyní v App
        if (!myApp.init()) { // Upraveno: init již nepřijímá GLFWwindow*
            throw std::runtime_error("Failed to initialize application");
        }
        myApp.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}