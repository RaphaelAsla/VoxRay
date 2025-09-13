#version 430 core

out vec4 FragColor;

uniform vec3 uColor;
uniform float uOpacity;

void main() {
    FragColor = vec4(uColor, uOpacity) * 2.5;
}
