#version 420 core
in vec3 FragPos;
in vec3 Normal;
in float Curvature;
out vec4 FragColor;
uniform int curvatureType;

vec3 mapToColor(float c) {
    c = clamp(c, 0.0, 1.0);
    float r, g, b;
    
    if (c < 0.125) {
        r = 0.0;
        g = 0.0;
        b = 0.5 + c / 0.125 * 0.5;
    } 
    else if (c < 0.375) {
        r = 0.0;
        g = (c - 0.125) / 0.25;
        b = 1.0;
    } 
    else if (c < 0.625) {
        r = (c - 0.375) / 0.25;
        g = 1.0;
        b = 1.0 - (c - 0.375) / 0.25;
    } 
    else if (c < 0.875) {
        r = 1.0;
        g = 1.0 - (c - 0.625) / 0.25;
        b = 0.0;
    } 
    else {
        r = 1.0 - (c - 0.875) / 0.125 * 0.5;
        g = 0.0;
        b = 0.0;
    }
    
    return vec3(r, g, b);
}

void main() {
    vec3 color = mapToColor(Curvature);
    FragColor = vec4(color, 1.0);
}