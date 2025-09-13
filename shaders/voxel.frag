#version 430 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
flat in int VoxelIndex;

out vec4 FragColor;

layout(std430, binding = 2) buffer ColorBuffer {
    vec4 colors[];
};

void main() {
    if (VoxelIndex < colors.length()) {
        FragColor = colors[VoxelIndex];
    } else {
        discard;
    }
}
