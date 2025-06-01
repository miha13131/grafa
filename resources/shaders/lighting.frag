#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Struktura pro bodové světlo
struct PointLight {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight light;
uniform vec3 viewPos;  // Pozice kamery
uniform sampler2D tex0; // Textura
uniform bool useTexture = false;
uniform vec4 u_diffuse_color = vec4(1.0, 1.0, 1.0, 1.0);

void main()
{
    // Normalizace normály
    vec3 norm = normalize(Normal);
    
    // Směr světla (od fragmentu ke světlu)
    vec3 lightDir = normalize(light.position - FragPos);
    
    // Směr pohledu (od fragmentu k pozorovateli/kameře)
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Směr odrazu (od fragmentu pryč)
    vec3 reflectDir = reflect(-lightDir, norm);
    
    // Výpočet ambient složky
    vec3 ambient = light.ambient * vec3(u_diffuse_color);
    
    // Výpočet diffuse složky
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(u_diffuse_color);
    
    // Výpočet specular složky
    float shininess = 32.0; // Ostrost odlesku
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * vec3(1.0, 1.0, 1.0); // Bílý odlesk
    
    // Útlum světla se vzdáleností
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // Aplikace útlumu na všechny složky osvětlení
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    // Kombinace všech složek osvětlení
    vec3 result = ambient + diffuse + specular;
    
    // Finální barva
    if (useTexture) {
        FragColor = vec4(result, 1.0) * texture(tex0, TexCoords);
    } else {
        FragColor = vec4(result, u_diffuse_color.a);
    }
}