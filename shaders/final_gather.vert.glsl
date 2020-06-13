#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos, 0.0f, 1.0f);
}