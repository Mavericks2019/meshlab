#version 430 core

in vec3 FragPos;
in vec2 TexCoord;

uniform float checkerFrequency;

out vec4 FragColor;

void main()
{
    const float pi = 3.14159265358979323846;
    float signal = sin(pi * checkerFrequency * TexCoord.x)
                 * sin(pi * checkerFrequency * TexCoord.y);
    float transitionWidth = max(fwidth(signal), 0.001);
    float checker = smoothstep(-transitionWidth, transitionWidth, signal);

    vec3 lightSquare = vec3(0.92, 0.92, 0.88);
    vec3 darkSquare = vec3(0.16, 0.20, 0.23);
    vec3 baseColor = mix(darkSquare, lightSquare, checker);

    vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    vec3 lightDirection = normalize(vec3(0.35, 0.55, 1.0));
    float lighting = 0.55 + 0.45 * abs(dot(faceNormal, lightDirection));
    FragColor = vec4(baseColor * lighting, 1.0);
}
