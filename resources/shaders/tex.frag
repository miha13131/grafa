#version 460 core
// (interpolated) input from previous pipeline stage
in vec3 FragPos;
in vec3 Normal;
in VS_OUT {
    vec2 texcoord;
} fs_in;
// uniform variables
uniform sampler2D tex0; // texture unit from C++
uniform vec4 u_diffuse_color = vec4(1.0f); // přidaný uniform pro barvu a průhlednost
// mandatory: final output color
out vec4 FragColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
void main() {
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
    
    // Výsledek
    vec3 result = (ambient + diffuse + specular);
    
    // Modifikujte původní barvu osvětlením
    vec4 texColor = texture(tex0, fs_in.texcoord);
    FragColor = vec4(result, 1.0) * u_diffuse_color * texColor;
}