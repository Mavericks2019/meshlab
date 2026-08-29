#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aComponentColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 ComponentColor;

void main()
{
    vec4 worldPosition = model * vec4(aPos, 1.0);
    FragPos = worldPosition.xyz;
    ComponentColor = aComponentColor;
    gl_Position = projection * view * worldPosition;
}
