#version 430 core
flat in int vertexID;

out vec4 FragColor;

void main()
{
    // 将顶点ID编码为颜色
    int r = (vertexID) & 0xFF;
    int g = (vertexID >> 8) & 0xFF;
    int b = (vertexID >> 16) & 0xFF;
    
    FragColor = vec4(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0, 1.0);
}