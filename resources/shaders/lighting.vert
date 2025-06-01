#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 uM_m;
uniform mat4 uV_m;
uniform mat4 uP_m;

void main()
{
    // Pozice fragmentu ve world space pro výpočet osvětlení
    FragPos = vec3(uM_m * vec4(aPos, 1.0));
    
    // Transformace normály - pro správnou transformaci bychom měli použít normálovou matici
    // (inverze transpozice model matice), ale pro jednoduchost použijeme jen část model matice
    Normal = mat3(transpose(inverse(uM_m))) * aNormal;
    
    // Předáváme texturové koordináty do fragment shaderu
    TexCoords = aTex;
    
    // Výsledná pozice vrcholu
    gl_Position = uP_m * uV_m * vec4(FragPos, 1.0);
}