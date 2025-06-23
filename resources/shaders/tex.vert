#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNorm;
layout(location = 1) in vec2 aTex;

uniform mat4 uP_m;
uniform mat4 uV_m;
uniform mat4 uM_m;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 texcoord;
} vs_out;

void main()
{
    vec4 worldPos = uM_m * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = mat3(transpose(inverse(uM_m))) * aNorm;
    vs_out.texcoord = aTex;
    gl_Position = uP_m * uV_m * worldPos;
}