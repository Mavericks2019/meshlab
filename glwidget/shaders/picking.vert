#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int vertexID;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexID = gl_VertexID;
}