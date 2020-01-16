#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D leftTriangleTex;
layout(set = 0, binding = 2) uniform sampler2D rightTriangleTex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(leftTriangleTex, fragTexCoord) + texture(rightTriangleTex, fragTexCoord);
}