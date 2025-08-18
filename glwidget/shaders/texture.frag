#version 420 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textureSampler;

void main() {
    vec4 texColor = texture(textureSampler, TexCoord);
    FragColor = texColor;
}