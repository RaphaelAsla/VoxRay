#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aInstancePos;
layout(location = 4) in vec3 aFaceNormal;
layout(location = 5) in int aVoxelIndex;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
flat out int VoxelIndex;

mat3 orientQuad(vec3 face_normal) {
    vec3 up = abs(face_normal.y) > 0.9 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, face_normal));
    up = normalize(cross(face_normal, right));
    return mat3(right, up, face_normal);
}

void main() {
    mat3 orientation = orientQuad(aFaceNormal);
    vec3 oriented_pos = orientation * aPos;

    FragPos = aInstancePos + oriented_pos;
    Normal = aFaceNormal;
    VoxelIndex = aVoxelIndex;

    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
