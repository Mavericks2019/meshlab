#version 420 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

void main() {
    vec3 norm = normalize(Normal);
    vec3 color = (norm + vec3(1.0)) * 0.5;
    
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(norm, lightDir), 0.2);
    
    FragColor = vec4(color * diff, 1.0);
}
