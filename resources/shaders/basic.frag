#version 460 core
in vec3 vertexPosition;
out vec4 FragColor;

void main() {
    // Duhový efekt na základě pozice
    vec3 rainbow = vec3(
        0.5 + 0.5 * sin(vertexPosition.x * 10.0),
        0.5 + 0.5 * sin(vertexPosition.y * 10.0 + 2.0),
        0.5 + 0.5 * sin(vertexPosition.z * 10.0 + 4.0)
    );
    
    FragColor = vec4(rainbow, 1.0);
}