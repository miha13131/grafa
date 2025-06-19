#pragma once  
#include <GL/glew.h>  
#include <GLFW/glfw3.h>  
#include <glm/glm.hpp>  
#include <glm/gtc/matrix_transform.hpp>  
#include <algorithm> // For std::max and std::clamp  
#include <opencv2/opencv.hpp> // For cv::Mat  

class Camera {  
public:  
   // Camera attributes  
   glm::vec3 Position{ 0.0f, 0.0f, 3.0f };  
   glm::vec3 Front{ 0.0f, 0.0f, -1.0f };  
   glm::vec3 Up{ 0.0f, 1.0f, 0.0f };  
   glm::vec3 Right{ 1.0f, 0.0f, 0.0f };  

   // Euler angles  
   float Yaw{ -90.0f };  // Rotation around Y axis (left/right)  
   float Pitch{ 0.0f };  // Rotation around X axis (up/down)  
   float Roll{ 0.0f };   // Rotation around Z axis (tilt)  

   // Camera parameters  
   float MovementSpeed{ 15.5f };  
   float MouseSensitivity{ 0.1f };
   // Jumping and gravity
   float VerticalVelocity{ 0.0f };
   bool  OnGround{ true };
   const float Gravity{ -30.81f };
   const float JumpSpeed{ 25.0f };

   Camera& operator=(const Camera& other) {
       if (this != &other) {
           Position = other.Position;
           Front = other.Front;
           Up = other.Up;
           Right = other.Right;
           Yaw = other.Yaw;
           Pitch = other.Pitch;
           Roll = other.Roll;
           MovementSpeed = other.MovementSpeed;
           MouseSensitivity = other.MouseSensitivity;
           VerticalVelocity = other.VerticalVelocity;
           OnGround = other.OnGround;
       }
       return *this;
   }

   Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f)) : Position(position) {  
       updateCameraVectors();  
   }  

   // Get View matrix  
   glm::mat4 GetViewMatrix() const {  
       return glm::lookAt(Position, Position + Front, Up);  
   }  

   // Process keyboard input  
   glm::vec3 ProcessKeyboard(GLFWwindow* window, float deltaTime) {  
       glm::vec3 direction{ 0.0f };  

       if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)  
           direction += Front;  
       if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)  
           direction -= Front;  
       if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)  
           direction -= Right;  
       if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
           direction += Right;

       // Jump when space is pressed and the camera is on the ground
       if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && OnGround) {
           VerticalVelocity = JumpSpeed;
           OnGround = false;
       }

       // Normalize direction vector if not zero  
       if (glm::length(direction) > 0.0f)  
           direction = glm::normalize(direction);  

       // Check if Shift is pressed for faster movement  
       float speed = MovementSpeed;  
       if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)  
           speed *= 2.5f;  // 2.5x faster with Shift  

       return direction * speed * deltaTime;  
   }  

   // Move camera  
   void Move(const glm::vec3& direction, cv::Mat& maze_map, float speed, cv::Mat& heightmap, float maxHeight, float deltaTime) {
       float tileSizeGL = 1.0f;  
       int width = heightmap.cols;  
       int height = heightmap.rows;  

       glm::vec3 newPos = Position + direction * speed;
       newPos.y += VerticalVelocity * deltaTime;
       VerticalVelocity += Gravity * deltaTime;
       float newX = newPos.x;
       float newZ = newPos.z;  

       int hm_x0 = static_cast<int>(newX / (15.0f * tileSizeGL) * (width - 1));  
       int hm_z0 = static_cast<int>(newZ / (15.0f * tileSizeGL) * (height - 1));  
       hm_x0 = std::clamp(hm_x0, 0, width - 2);  
       hm_z0 = std::clamp(hm_z0, 0, height - 2);  

       float fx = (newX / (15.0f * tileSizeGL) * (width - 1)) - hm_x0;  
       float fz = (newZ / (15.0f * tileSizeGL) * (height - 1)) - hm_z0;  
       fx = std::clamp(fx, 0.0f, 1.0f);  
       fz = std::clamp(fz, 0.0f, 1.0f);  

       float h00 = static_cast<float>(heightmap.at<uchar>(hm_z0, hm_x0)) / 255.0f * maxHeight;  
       float h01 = static_cast<float>(heightmap.at<uchar>(hm_z0, hm_x0 + 1)) / 255.0f * maxHeight;  
       float h10 = static_cast<float>(heightmap.at<uchar>(hm_z0 + 1, hm_x0)) / 255.0f * maxHeight;  
       float h11 = static_cast<float>(heightmap.at<uchar>(hm_z0 + 1, hm_x0 + 1)) / 255.0f * maxHeight;  

       float terrainHeight = h00 * (1 - fx) * (1 - fz) +  
           h01 * fx * (1 - fz) +  
           h10 * (1 - fx) * fz +  
           h11 * fx * fz;  

       if (newPos.y < terrainHeight + 0.5f) {
           newPos.y = terrainHeight + 0.5f; // Offset 0.5 nad terénem
           if (VerticalVelocity <= 0.0f) {
               VerticalVelocity = 0.0f;
               OnGround = true;
           }
       }
       else {
           OnGround = false;
       }
       Position = newPos;
   }



   // Process mouse movement  
   void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {  
       xoffset *= MouseSensitivity;  
       yoffset *= MouseSensitivity;  

       Yaw += xoffset;  
       Pitch += yoffset;  

       if (constrainPitch) {  
           if (Pitch > 89.0f)  
               Pitch = 89.0f;  
           if (Pitch < -89.0f)  
               Pitch = -89.0f;  
       }  

       updateCameraVectors();  
   }  

private:  
   void updateCameraVectors() {  
       glm::vec3 front;  
       front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));  
       front.y = sin(glm::radians(Pitch));  
       front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));  
       Front = glm::normalize(front);  
       Right = glm::normalize(glm::cross(Front, glm::vec3(0.0f, 1.0f, 0.0f)));  
       Up = glm::normalize(glm::cross(Right, Front));  
   }  
};