#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aLife;

uniform mat4 uP_m = mat4(1.0);
uniform mat4 uV_m = mat4(1.0);
uniform mat4 uM_m = mat4(1.0);

out float vLife;

void main() {
    gl_Position = uP_m * uV_m * uM_m * vec4(aPos, 1.0);
    gl_PointSize = 5.0;
    vLife = aLife;
}
