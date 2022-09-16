#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in GS_OUT{
    vec4 color;
} gs_out;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = gs_out.color;
}