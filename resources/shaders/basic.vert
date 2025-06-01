#version 460 core
in vec3 attribute_Position;
out vec3 vertexPosition;
uniform mat4 uP_m = mat4(1.0);  // Projekční matice
uniform mat4 uV_m = mat4(1.0);  // View matice (kamera)
uniform mat4 uM_m = mat4(1.0);  // Model matice (pozice, rotace, měřítko objektu)

void main() {
    // Transformace vrcholu pomocí matic
    vec4 worldPosition = uM_m * vec4(attribute_Position, 1.0);
    vec4 viewPosition = uV_m * worldPosition;
    gl_Position = uP_m * viewPosition;
    
    // Předání pozice do fragment shaderu
    vertexPosition = worldPosition.xyz;
}