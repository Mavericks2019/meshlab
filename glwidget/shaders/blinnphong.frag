#version 430 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;
uniform vec3 lightPositions[3];
uniform vec3 lightColors[3];
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool specularEnabled;

void main() {
   float ambientStrength = 0.1;
   vec3 result = vec3(0.0);
   
   for(int i = 0; i < 3; i++) {
       vec3 ambient = ambientStrength * lightColors[i];
       
       vec3 norm = normalize(Normal);
       vec3 lightDir = normalize(lightPositions[i] - FragPos);
       float diff = max(dot(norm, lightDir), 0.0);
       vec3 diffuse = diff * lightColors[i];
       
       vec3 specular = vec3(0.0);
       if (specularEnabled) {
           float specularStrength = 0.5;
           vec3 viewDir = normalize(viewPos - FragPos);
           vec3 halfwayDir = normalize(lightDir + viewDir);
           float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
           specular = specularStrength * spec * lightColors[i];
       }
       
       result += (ambient + diffuse + specular) * objectColor;
   }
   
   FragColor = vec4(result, 1.0);
}