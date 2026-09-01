#version 430 core

in vec3 FragPos;

uniform vec4 overlayColor;

out vec4 FragColor;

void main()
{
    vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    vec3 lightDirection = normalize(vec3(0.35, 0.55, 1.0));
    float lighting = 0.68 + 0.32 * abs(dot(faceNormal, lightDirection));
    FragColor = vec4(overlayColor.rgb * lighting, overlayColor.a);
}
