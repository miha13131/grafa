#version 460 core
out vec4 FragColor;

uniform vec3 lightColor;

void main()
{
    // Jednoduše nastaví barvu světla (obvykle jasně bílou nebo žlutou)
    FragColor = vec4(lightColor, 1.0);
}