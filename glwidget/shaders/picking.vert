#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int vertexID;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 viewPos = view * worldPos;
    gl_Position = projection * viewPos;
    gl_PointSize = 5.0; // 可选：增加点大小以提高拾取精度
    vertexID = gl_VertexID;
}