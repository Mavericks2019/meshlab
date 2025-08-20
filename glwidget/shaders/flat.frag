#version 430 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPositions[3];
uniform vec3 lightColors[3];
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool specularEnabled;

void main()
{
    // 使用面的法向量（通过求导计算得到）
    vec3 fdx = dFdx(FragPos);
    vec3 fdy = dFdy(FragPos);
    vec3 faceNormal = normalize(cross(fdx, fdy));
    
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < 3; i++) {
        // 环境光
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColors[i];
        
        // 漫反射
        vec3 lightDir = normalize(lightPositions[i] - FragPos);
        float diff = max(dot(faceNormal, lightDir), 0.0);
        vec3 diffuse = diff * lightColors[i];
        
        // 镜面反射
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, faceNormal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColors[i];
        
        if (!specularEnabled) {
            specular = vec3(0.0);
        }
        
        result += (ambient + diffuse + specular) * objectColor;
    }
    
    FragColor = vec4(result, 1.0);
}